
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

    std::vector<std::function<void(std::vector<std::string>)>> fileDropListeners;

    int width = 0;
    int height = 0;

    static Renderer *Instance();
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void renderOneFrame(std::function<void(void)> update, std::function<void(void)> render);
    void setSize(int width, int height);

    void addFileDropCallback(std::function<void(std::vector<std::string>)> callback);

  private:
    Renderer();
};