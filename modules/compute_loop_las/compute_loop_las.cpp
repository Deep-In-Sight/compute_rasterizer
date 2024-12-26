
#include <compute_loop_las/compute_loop_las.h>
#include <compute_loop_las/compute_loop_las_shaders.h>

#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "glm/common.hpp"
#include "glm/matrix.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "unsuck.hpp"

#include "Box.h"
#include "Camera.h"
#include "Debug.h"
#include "Frustum.h"
#include "GLTimerQueries.h"
#include "Method.h"
#include "PointCloudRenderer.h"
#include "PointManager.h"
#include "Shader.h"
#include "compute_loop_las_shaders.h"
#include <Framebuffer.h>
#include <Texture.h>

using namespace std;
using namespace std::chrono_literals;

using glm::ivec2;

ComputeLoopLas::ComputeLoopLas(PointCloudRenderer *renderer, shared_ptr<PointManager> pointManager)
{

    this->name = "loop_las";
    this->description = R"ER01(
- Each thread renders X points.
- Loads points from LAS file
- encodes point coordinates in 10+10+10 bits
- Workgroup picks 10, 20 or 30 bit precision
  depending on screen size of bounding box
		)ER01";
    this->pointManager = pointManager;
    this->group = "10-10-10 bit encoded";

    csRender = new Shader({{render_cs, GL_COMPUTE_SHADER}});
    csResolve = new Shader({{resolve_cs, GL_COMPUTE_SHADER}});

    ssFramebuffer = createBuffer(8 * 2048 * 2048);

    this->renderer = renderer;

    ssFilesBuffer = make_shared<Buffer>(10'000 * 128);

    ssDebug = createBuffer(256);
    ssBoundingBoxes = createBuffer(48 * 1'000'000);
    ssFiles = createBuffer(ssFilesBuffer->size);
    uniformBuffer = createUniformBuffer(512);

    GLuint zero = 0;
    glClearNamedBufferData(ssDebug.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssBoundingBoxes.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssFiles.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
}

void ComputeLoopLas::update(PointCloudRenderer *renderer)
{
}

void ComputeLoopLas::render(PointCloudRenderer *renderer)
{

    GLTimerQueries::timestamp("compute-loop-start");

    pointManager->uploadQueuedPoints();

    if (pointManager->numPointsLoaded == 0)
    {
        return;
    }

    auto fbo = renderer->views[0].framebuffer;

    // resize framebuffer storage, if necessary
    if (ssFramebuffer.size < 8 * fbo->width * fbo->height)
    {

        glDeleteBuffers(1, &ssFramebuffer.handle);

        // make new buffer a little larger to have some reserves when users enlarge the window
        int newBufferSize = 1.5 * double(8 * fbo->width * fbo->height);

        ssFramebuffer = createBuffer(newBufferSize);
    }

    // Update Uniform Buffer
    {
        mat4 world;
        mat4 view = renderer->camera->view;
        mat4 proj = renderer->camera->proj;
        mat4 worldView = view * world;
        mat4 worldViewProj = proj * view * world;

        uniformData.world = world;
        uniformData.view = view;
        uniformData.proj = proj;
        uniformData.transform = worldViewProj;
        if (Debug::updateFrustum)
        {
            uniformData.transformFrustum = worldViewProj;
        }
        uniformData.pointsPerThread = POINTS_PER_THREAD;
        uniformData.numPoints = pointManager->numPointsLoaded;
        uniformData.enableFrustumCulling = Debug::frustumCullingEnabled ? 1 : 0;
        uniformData.showBoundingBox = Debug::showBoundingBox ? 1 : 0;
        uniformData.imageSize = {fbo->width, fbo->height};
        uniformData.colorizeChunks = Debug::colorizeChunks;
        uniformData.colorizeOverdraw = Debug::colorizeOverdraw;

        glNamedBufferSubData(uniformBuffer.handle, 0, sizeof(UniformData), &uniformData);
    }

    { // update file buffer

        for (int i = 0; i < pointManager->boxes.size(); i++)
        {
            auto box_min = pointManager->boxes[i].min;
            dmat4 world = glm::translate(dmat4(), box_min);
            dmat4 view = renderer->camera->view;
            dmat4 proj = renderer->camera->proj;
            dmat4 worldView = view * world;
            dmat4 worldViewProj = proj * view * world;

            mat4 transform = worldViewProj;
            mat4 fWorld = world;

            memcpy(ssFilesBuffer->data_u8 + 256 * i + 0, glm::value_ptr(transform), 64);

            if (Debug::updateFrustum)
            {
                memcpy(ssFilesBuffer->data_u8 + 256 * i + 64, glm::value_ptr(transform), 64);
            }

            memcpy(ssFilesBuffer->data_u8 + 256 * i + 128, glm::value_ptr(fWorld), 64);
        }

        glNamedBufferSubData(ssFiles.handle, 0, 256 * pointManager->boxes.size(), ssFilesBuffer->data);
    }

    if (Debug::enableShaderDebugValue)
    {
        DebugData data;
        data.enabled = true;

        glNamedBufferSubData(ssDebug.handle, 0, sizeof(DebugData), &data);
    }

    // RENDER
    if (csRender->program != -1)
    {
        GLTimerQueries::timestamp("draw-start");

        glUseProgram(csRender->program);

        auto &viewLeft = renderer->views[0];

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssFramebuffer.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 30, ssDebug.handle);
        glBindBufferBase(GL_UNIFORM_BUFFER, 31, uniformBuffer.handle);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 40, pointManager->ssBatches.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 41, pointManager->ssXyzHig.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 42, pointManager->ssXyzMed.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 43, pointManager->ssXyzLow.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 44, pointManager->ssColors.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 45, ssFiles.handle);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 50, ssBoundingBoxes.handle);

        glBindImageTexture(0, fbo->colorAttachments[0]->handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);

        // int numBatches = ceil(double(pointManager->numPointsLoaded) / double(POINTS_PER_WORKGROUP));
        int numBatches = pointManager->numBatchesLoaded;

        glDispatchCompute(numBatches, 1, 1);

        GLTimerQueries::timestamp("draw-end");
    }

    // RESOLVE
    if (csResolve->program != -1)
    {
        GLTimerQueries::timestamp("resolve-start");

        glUseProgram(csResolve->program);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssFramebuffer.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 30, ssDebug.handle);
        glBindBufferBase(GL_UNIFORM_BUFFER, 31, uniformBuffer.handle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 44, pointManager->ssColors.handle);

        glBindImageTexture(0, fbo->colorAttachments[0]->handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);

        int groups_x = ceil(float(fbo->width) / 16.0f);
        int groups_y = ceil(float(fbo->height) / 16.0f);
        glDispatchCompute(groups_x, groups_y, 1);

        GLTimerQueries::timestamp("resolve-end");
    }

    // READ DEBUG VALUES
    if (Debug::enableShaderDebugValue)
    {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        DebugData data;
        glGetNamedBufferSubData(ssDebug.handle, 0, sizeof(DebugData), &data);

        auto dbg = Debug::getInstance();

        dbg->pushFrameStat("#nodes processed", formatNumber(data.numNodesProcessed));
        dbg->pushFrameStat("#nodes rendered", formatNumber(data.numNodesRendered));
        dbg->pushFrameStat("#points processed", formatNumber(data.numPointsProcessed));
        dbg->pushFrameStat("#points rendered", formatNumber(data.numPointsRendered));
        dbg->pushFrameStat("divider", "");
        dbg->pushFrameStat("#points visible", formatNumber(data.numPointsVisible));

        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    { // CLEAR
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        GLuint zero = 0;
        float inf = -Infinity;
        GLuint intbits;
        memcpy(&intbits, &inf, 4);

        glClearNamedBufferSubData(ssFramebuffer.handle, GL_R32UI, 0, fbo->width * fbo->height * 8, GL_RED,
                                  GL_UNSIGNED_INT, &intbits);
        glClearNamedBufferData(ssDebug.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
        glClearNamedBufferSubData(ssBoundingBoxes.handle, GL_R32UI, 0, 48, GL_RED, GL_UNSIGNED_INT, &zero);

        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    GLTimerQueries::timestamp("compute-loop-end");
}