
#include "Renderer.h"
#include "GL/glew.h"
#include "GLTimerQueries.h"
#include "Runtime.h"
#include <Camera.h>
#include <Debug.h>
#include <Framebuffer.h>
#include <OrbitControls.h>
#include <Texture.h>
#include <filesystem>

namespace fs = std::filesystem;

Renderer *Renderer::Instance()
{
    static Renderer instance;
    return &instance;
}

Renderer::Renderer()
{
    controls = std::make_shared<OrbitControls>();
    camera = make_shared<Camera>();
    controls->setCamera(camera.get());

    View view1;
    view1.framebuffer = Framebuffer::create(128, 128);
    views.push_back(view1);
}

void Renderer::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
    camera->setSize(width, height);
    views[0].framebuffer->setSize(width, height);
}

void Renderer::setTargetFbo(unsigned int fboId)
{
    targetFboId = fboId;
}

void Renderer::renderOneFrame()
{
    GLTimerQueries::frameStart();
    Debug::clearFrameStats();

    EventQueue::instance()->process();

    {
        controls->update();
    }

    { // UPDATE & RENDER
        auto selected = Runtime::getSelectedMethod();
        if (selected)
        {
            selected->update(this);
            selected->render(this);
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