
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "glm/matrix.hpp"

struct Camera;
struct Framebuffer;
struct ComputeLoopLas;

struct View
{
    glm::dmat4 view;
    glm::dmat4 proj;
    std::shared_ptr<Framebuffer> framebuffer = nullptr;
};

struct PointCloudRenderer
{
    PointCloudRenderer();
    std::shared_ptr<Camera> camera = nullptr;
    std::shared_ptr<ComputeLoopLas> computeLoopLas = nullptr;
    std::vector<View> views;

    int width = 0;
    int height = 0;

    void renderOneFrame();
    void setTargetFbo(unsigned int fboId);
    void setSize(int width, int height);

  private:
    unsigned int targetFboId = 0;
};