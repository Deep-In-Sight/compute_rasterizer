#include <CameraController.h>
#include <GL/glew.h>
#include <LasLoaderSparse.h>
#include <PointCloudItem.h>
#include <PointCloudQuickRenderer.h>
#include <PointCloudRenderer.h>
#include <QMimeData>
#include <QOpenGLFramebufferObject>
#include <Runtime.h>
#include <compute_loop_las/compute_loop_las.h>
#include <iostream>

PointCloudQuickRenderer::PointCloudQuickRenderer()
{
    initialize();
    pcdRenderer = std::make_shared<PointCloudRenderer>();
    lasLoader = std::make_shared<LasLoaderSparse>();
    cameraController = std::make_shared<CameraController>(pcdRenderer->camera.get());
    pcdRenderer->computeLoopLas = std::make_shared<ComputeLoopLas>(pcdRenderer.get(), lasLoader);
}

void PointCloudQuickRenderer::addLasFiles(const std::vector<std::string> &lasPaths)
{
    lasLoader->add(lasPaths);
}

void PointCloudQuickRenderer::getSceneBox(glm::dvec3 &center, glm::dvec3 &size)
{
    center = lasLoader->boxCenter;
    size = lasLoader->boxSize;
}

CameraController *PointCloudQuickRenderer::getCameraController()
{
    return cameraController.get();
}

void PointCloudQuickRenderer::synchronize(QQuickFramebufferObject *item)
{
    auto *quickItem = static_cast<PointCloudItem *>(item);

    if (quickItem->boundingRect().size() != viewportSize.size())
    {
        viewportSize = quickItem->boundingRect();
        pcdRenderer->setSize(viewportSize.width(), viewportSize.height());
    }

    if (framebufferObject() != nullptr)
    {
        pcdRenderer->setTargetFbo(framebufferObject()->handle());
    }
}

static void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                   const GLchar *message, const void *userParam)
{

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW ||
        severity == GL_DEBUG_SEVERITY_MEDIUM)
    {
        return;
    }

    cout << "OPENGL DEBUG CALLBACK: " << message << endl;
}

void PointCloudQuickRenderer::initialize()
{
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
    glDebugMessageCallback(debugCallback, NULL);
}

void PointCloudQuickRenderer::render()
{
    pcdRenderer->renderOneFrame();
}