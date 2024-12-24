
#include "PointCloudRenderer.h"
#include "GL/glew.h"
#include "GLTimerQueries.h"
#include "Runtime.h"
#include <Camera.h>
#include <Debug.h>
#include <Framebuffer.h>
#include <Texture.h>
#include <compute_loop_las/compute_loop_las.h>
#include <filesystem>

namespace fs = std::filesystem;

PointCloudRenderer::PointCloudRenderer()
{
    camera = make_shared<Camera>();

    View view1;
    view1.framebuffer = Framebuffer::create(128, 128);
    views.push_back(view1);
}

void PointCloudRenderer::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
    camera->setSize(width, height);
    views[0].framebuffer->setSize(width, height);
}

void PointCloudRenderer::setTargetFbo(unsigned int fboId)
{
    targetFboId = fboId;
}

void PointCloudRenderer::renderOneFrame()
{
    GLTimerQueries::frameStart();
    Debug::clearFrameStats();

    EventQueue::instance()->process();

    { // UPDATE & RENDER
        if (computeLoopLas != nullptr)
        {
            computeLoopLas->update(this);
            computeLoopLas->render(this);
        }
    }

    {
        auto source = views[0].framebuffer;
        glBlitNamedFramebuffer(source->handle, targetFboId, 0, 0, source->width, source->height, 0, 0,
                               0 + source->width, 0 + source->height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    // FINISH FRAME
    GLTimerQueries::frameEnd();
}