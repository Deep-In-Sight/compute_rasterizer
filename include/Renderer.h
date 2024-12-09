
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "glm/matrix.hpp"

struct Camera;
struct OrbitControls;
struct Framebuffer;

struct View
{
    glm::dmat4 view;
    glm::dmat4 proj;
    std::shared_ptr<Framebuffer> framebuffer = nullptr;
};

struct Renderer
{

    std::shared_ptr<Camera> camera = nullptr;
    std::shared_ptr<OrbitControls> controls = nullptr;
    std::vector<View> views;

    int width = 0;
    int height = 0;

    /**
     * @brief Be mindful of the initialization of this class
     * First call to this function should be placed where an OpenGL context is available
     *
     * @return Renderer*
     */
    static Renderer *Instance();
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void renderOneFrame();
    void setSize(int width, int height);

  private:
    Renderer();
};