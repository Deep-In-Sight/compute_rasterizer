
#pragma once

#include "glm/common.hpp"
#include "glm/matrix.hpp"
#include <GLBuffer.h>
#include <Method.h>
#include <Shader.h>
#include <memory>
#include <string>

struct Buffer;
struct LasLoaderSparse;
struct PointCloudRenderer;

struct ComputeLoopLas : public Method
{
    struct UniformData
    {
        glm::mat4 world;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 transform;
        glm::mat4 transformFrustum;
        int pointsPerThread;
        int enableFrustumCulling;
        int showBoundingBox;
        int numPoints;
        glm::ivec2 imageSize;
        int colorizeChunks;
        int colorizeOverdraw;
    };

    struct DebugData
    {
        uint32_t value = 0;
        bool enabled = false;
        uint32_t numPointsProcessed = 0;
        uint32_t numNodesProcessed = 0;
        uint32_t numPointsRendered = 0;
        uint32_t numNodesRendered = 0;
        uint32_t numPointsVisible = 0;
    };

    std::string source = "";
    Shader *csRender = nullptr;
    Shader *csResolve = nullptr;

    GLBuffer ssFramebuffer;
    GLBuffer ssDebug;
    GLBuffer ssBoundingBoxes;
    GLBuffer ssFiles;
    GLBuffer uniformBuffer;
    UniformData uniformData;
    std::shared_ptr<Buffer> ssFilesBuffer;

    std::shared_ptr<LasLoaderSparse> las = nullptr;

    PointCloudRenderer *renderer = nullptr;

    ComputeLoopLas(PointCloudRenderer *renderer, shared_ptr<LasLoaderSparse> las);
    void update(PointCloudRenderer *renderer);
    void render(PointCloudRenderer *renderer);
};