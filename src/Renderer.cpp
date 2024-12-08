
#include "Renderer.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "GLTimerQueries.h"
#include "Runtime.h"
#include <Camera.h>
#include <Debug.h>
#include <Framebuffer.h>
#include <ImGuiOverlay.h>
#include <OrbitControls.h>
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

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

void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    cout << "key: " << key << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << endl;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    Runtime::keyStates[key] = action;

    cout << key << endl;
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    Runtime::mousePosition = {xpos, ypos};

    Renderer::Instance()->controls->onMouseMove(xpos, ypos);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    Renderer::Instance()->controls->onMouseScroll(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{

    cout << "start button: " << button << ", action: " << action << ", mods: " << mods << endl;

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    cout << "end button: " << button << ", action: " << action << ", mods: " << mods << endl;

    if (action == 1)
    {
        Runtime::mouseButtons = Runtime::mouseButtons | (1 << button);
    }
    else if (action == 0)
    {
        uint32_t mask = ~(1 << button);
        Runtime::mouseButtons = Runtime::mouseButtons & mask;
    }

    Renderer::Instance()->controls->onMouseButton(button, action, mods);
}

void drop_callback(GLFWwindow *, int count, const char **paths)
{
    vector<string> files;
    for (int i = 0; i < count; i++)
    {
        string file = paths[i];
        files.push_back(file);
    }

    for (auto &listener : Renderer::Instance()->fileDropListeners)
    {
        listener(files);
    }
}

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

    init();

    View view1;
    view1.framebuffer = Framebuffer::create(128, 128);
    View view2;
    view2.framebuffer = Framebuffer::create(128, 128);
    views.push_back(view1);
    views.push_back(view2);
}

void Renderer::init()
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        // Initialization failed
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, true);

    int numMonitors;
    GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);

    cout << "<create windows>" << endl;
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[0]);

        window = glfwCreateWindow(mode->width - 100, mode->height - 100, "Simple example", nullptr, nullptr);

        if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwSetWindowPos(window, 50, 50);
    }

    cout << "<set input callbacks>" << endl;
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetDropCallback(window, drop_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "glew error: %s\n", glewGetErrorString(err));
    }

    cout << "<glewInit done> " << "(" << now() << ")" << endl;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
    glDebugMessageCallback(debugCallback, NULL);

    ImGuiInit(window);
}

void Renderer::loop(function<void(void)> update, function<void(void)> render)
{
    while (!glfwWindowShouldClose(window))
    {
        GLTimerQueries::frameStart();
        Debug::clearFrameStats();

        // WINDOW
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        camera->setSize(width, height);
        this->width = width;
        this->height = height;

        EventQueue::instance()->process();

        {
            controls->update();
        }

        { // UPDATE & RENDER
            update();
            render();
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, this->width, this->height);
            auto source = views[0].framebuffer;
            glBlitNamedFramebuffer(source->handle, 0, 0, 0, source->width, source->height, 0, 0, 0 + source->width,
                                   0 + source->height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }

        ImGuiUpdate();

        // FINISH FRAME
        GLTimerQueries::frameEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGuiDestroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void Renderer::addFileDropCallback(std::function<void(std::vector<std::string>)> callback)
{
    fileDropListeners.push_back(callback);
}