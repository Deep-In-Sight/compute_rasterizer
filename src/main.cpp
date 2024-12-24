#include "LasLoaderSparse.h"
#include "PointCloudRenderer.h"
#include "Runtime.h"
#include "compute_loop_las/compute_loop_las.h"
#include <ImGuiOverlay.h>
#include <OrbitControls.h>
#include <imgui.h>
#include <iostream>

using namespace std;

int numPoints = 1'000'000;
OrbitControls *orbitControls = nullptr;
PointCloudRenderer *renderer = nullptr;

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

    if (orbitControls != nullptr)
    {
        orbitControls->onMouseMove(xpos, ypos);
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    if (orbitControls != nullptr)
    {
        orbitControls->onMouseScroll(xoffset, yoffset);
    }
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

    if (orbitControls != nullptr)
    {
        orbitControls->onMouseButton(button, action, mods);
    }
}

std::function<void(std::vector<std::string> &)> lasFilesDroppedCallback;
std::shared_ptr<LasLoaderSparse> lasLoaderSparse = nullptr;
void drop_callback(GLFWwindow *, int count, const char **paths)
{
    vector<string> files;
    for (int i = 0; i < count; i++)
    {
        string file = paths[i];
        if (iEndsWith(file, "las") || iEndsWith(file, "laz"))
        {
            files.push_back(file);
        }
    }
    if (lasLoaderSparse != nullptr)
        lasLoaderSparse->add(files);
}

void window_size_callback(GLFWwindow *window, int width, int height)
{
    if (renderer != nullptr)
        renderer->setSize(width, height);
}

int main()
{
    GLFWwindow *window = nullptr;
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
    int windowWidth = 800, windowHeight = 600;

    cout << "<create windows>" << endl;
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[0]);

        // window = glfwCreateWindow(mode->width - 100, mode->height - 100, "Simple example", nullptr, nullptr);
        window = glfwCreateWindow(windowWidth, windowHeight, "Simple example", nullptr, nullptr);

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
    glfwSetWindowSizeCallback(window, window_size_callback);

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

    cout << std::setprecision(2) << std::fixed;

    renderer = new PointCloudRenderer();
    orbitControls = new OrbitControls();
    orbitControls->setCamera(renderer->camera.get());
    lasLoaderSparse = make_shared<LasLoaderSparse>();

    Runtime::lasLoaderSparse = lasLoaderSparse;

    { // 4-4-4 byte format
        renderer->computeLoopLas = std::make_shared<ComputeLoopLas>(renderer, lasLoaderSparse);
    }

    ImGuiInit(window);
    renderer->setTargetFbo(0); // on screen
    renderer->setSize(windowWidth, windowHeight);

    while (!glfwWindowShouldClose(window))
    {
        renderer->renderOneFrame();
        ImGuiUpdate();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGuiDestroy();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
