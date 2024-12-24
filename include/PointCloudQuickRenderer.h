#pragma once

#include <QQuickFramebufferObject>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct PointCloudRenderer;
struct LasLoaderSparse;
struct CameraController;

class PointCloudQuickRenderer : public QQuickFramebufferObject::Renderer
{

  public:
    PointCloudQuickRenderer();

    void addLasFiles(const std::vector<std::string> &lasPaths);
    CameraController *getCameraController();
    void getSceneBox(glm::dvec3 &center, glm::dvec3 &size);

  private:
    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;

  public:
    void initialize();
    QRectF viewportSize;
    std::shared_ptr<PointCloudRenderer> pcdRenderer;
    std::shared_ptr<LasLoaderSparse> lasLoader;
    std::shared_ptr<CameraController> cameraController;
};