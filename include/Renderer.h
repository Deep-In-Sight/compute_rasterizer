
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "implot_internal.h"

#include "glm/common.hpp"
#include "glm/matrix.hpp"
#include <glm/gtx/transform.hpp>

// #include "VrRuntime.h"
#include "Box.h"
#include "Camera.h"
#include "Debug.h"
#include "Framebuffer.h"
#include "GLBuffer.h"
#include "OrbitControls.h"
#include "Texture.h"
#include "unsuck.hpp"

using namespace std;
using glm::dmat4;
using glm::dvec3;
using glm::dvec4;

// ScrollingBuffer from ImPlot implot_demo.cpp.
// MIT License
// url: https://github.com/epezent/implot
struct ScrollingBuffer
{
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer()
    {
        MaxSize = 2000;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (Data.size() > 0)
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

struct DrawQueue
{
    vector<Box> boxes;
    vector<Box> boundingBoxes;

    void clear()
    {
        boxes.clear();
        boundingBoxes.clear();
    }
};

struct View
{
    dmat4 view;
    dmat4 proj;
    shared_ptr<Framebuffer> framebuffer = nullptr;
};

struct Renderer
{

    GLFWwindow *window = nullptr;
    double fps = 0.0;
    int64_t frameCount = 0;

    shared_ptr<Camera> camera = nullptr;
    shared_ptr<OrbitControls> controls = nullptr;

    bool vrEnabled = false;

    vector<View> views;

    vector<function<void(vector<string>)>> fileDropListeners;

    int width = 0;
    int height = 0;
    string selectedMethod = "";

    DrawQueue drawQueue;

    Renderer();

    void init();

    void loop(function<void(void)> update, function<void(void)> render);

    void drawBox(glm::dvec3 position, glm::dvec3 scale, glm::ivec3 color);

    void drawBoundingBox(glm::dvec3 position, glm::dvec3 scale, glm::ivec3 color);
    void drawBoundingBoxes(Camera *camera, GLBuffer buffer);

    void drawPoints(void *points, int numPoints);

    void drawPoints(GLuint vao, GLuint vbo, int numPoints);

    void addFileDropCallback(function<void(vector<string>)> callback)
    {
        fileDropListeners.push_back(callback);
    }
};