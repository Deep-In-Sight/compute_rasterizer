
#include <filesystem>

#include "GLTimerQueries.h"
#include "Renderer.h"
#include "Runtime.h"
#include "drawBoundingBoxes.h"
#include "drawBoundingBoxesIndirect.h"
#include "drawBoxes.h"
#include "drawPoints.h"

namespace fs = std::filesystem;

auto _controls = make_shared<OrbitControls>();

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

    _controls->onMouseMove(xpos, ypos);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    _controls->onMouseScroll(xoffset, yoffset);
}

void drop_callback(GLFWwindow *window, int count, const char **paths)
{
    for (int i = 0; i < count; i++)
    {
        cout << paths[i] << endl;
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

    _controls->onMouseButton(button, action, mods);
}

Renderer::Renderer()
{
    this->controls = _controls;
    camera = make_shared<Camera>();
    _controls->setCamera(camera.get());

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
    // glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, true);

    int numMonitors;
    GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);

    cout << "<create windows>" << endl;
    // if (numMonitors > 1) {
    //	const GLFWvidmode * modeLeft = glfwGetVideoMode(monitors[0]);
    //	const GLFWvidmode * modeRight = glfwGetVideoMode(monitors[1]);

    //	window = glfwCreateWindow(modeRight->width, modeRight->height - 300, "Simple example", nullptr, nullptr);

    //	if (!window) {
    //		glfwTerminate();
    //		exit(EXIT_FAILURE);
    //	}

    //	glfwSetWindowPos(window, modeLeft->width, 0);
    //}
    // else
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
    // glfwSetDropCallback(window, drop_callback);

    static Renderer *ref = this;
    glfwSetDropCallback(window, [](GLFWwindow *, int count, const char **paths) {
        vector<string> files;
        for (int i = 0; i < count; i++)
        {
            string file = paths[i];
            files.push_back(file);
        }

        for (auto &listener : ref->fileDropListeners)
        {
            listener(files);
        }
    });

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

    { // SETUP IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 450");
        ImGui::StyleColorsDark();
    }
}

void ImGuiUpdate();
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

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void ImGuiUpdate()
{
    // IMGUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto windowSize_perf = ImVec2(490, 340);
    auto &io = ImGui::GetIO();

    { // RENDER IMGUI PERFORMANCE WINDOW
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %.1f", io.Framerate);

        static float history = 2.0f;
        static ScrollingBuffer sFrames;
        static ScrollingBuffer s60fps;
        static ScrollingBuffer s120fps;
        float t = now();

        {
            auto durations = GLTimerQueries::instance()->durations;
            auto stats = GLTimerQueries::instance()->stats;

            if (stats.find("frame") != stats.end())
            {
                auto stat = stats["frame"];
                double avg = stat.sum / stat.count;

                sFrames.AddPoint(t, avg);
            }
            else
            {
                // add bogus entry until proper frame times become available
                sFrames.AddPoint(t, 100.0);
            }
        }

        // sFrames.AddPoint(t, 1000.0f * timeSinceLastFrame);
        s60fps.AddPoint(t, 1000.0f / 60.0f);
        s120fps.AddPoint(t, 1000.0f / 120.0f);
        static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_NoTickLabels;
        ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
        ImPlot::SetNextPlotLimitsY(0, 30, ImGuiCond_Always);

        if (ImPlot::BeginPlot("Timings", nullptr, nullptr, ImVec2(-1, 200)))
        {

            auto x = &sFrames.Data[0].x;
            auto y = &sFrames.Data[0].y;
            ImPlot::PlotShaded("frame time(ms)", x, y, sFrames.Data.size(), -Infinity, sFrames.Offset,
                               2 * sizeof(float));

            ImPlot::PlotLine("16.6ms (60 FPS)", &s60fps.Data[0].x, &s60fps.Data[0].y, s60fps.Data.size(), s60fps.Offset,
                             2 * sizeof(float));
            ImPlot::PlotLine(" 8.3ms (120 FPS)", &s120fps.Data[0].x, &s120fps.Data[0].y, s120fps.Data.size(),
                             s120fps.Offset, 2 * sizeof(float));

            ImPlot::EndPlot();
        }

        {

            static vector<Duration> s_durations;
            static unordered_map<string, GLTStats> s_stats;
            static float toggle = 0.0;

            // update duration only once per second
            // updating at full speed makes it hard to read it
            if (toggle > 1.0)
            {
                s_durations = GLTimerQueries::instance()->durations;
                s_stats = GLTimerQueries::instance()->stats;

                toggle = 0.0;
            }
            toggle = toggle + io.DeltaTime;

            string text = "Durations: \n";
            // auto durations = GLTimerQueries::instance()->durations;
            // auto stats = GLTimerQueries::instance()->stats;

            for (auto duration : s_durations)
            {

                auto stat = s_stats[duration.label];
                double avg = stat.sum / stat.count;
                double min = stat.min;
                double max = stat.max;

                text = text + rightPad(duration.label + ":", 30);
                text = text + "avg(" + formatNumber(avg, 3) + ") ";
                text = text + "min(" + formatNumber(min, 3) + ") ";
                text = text + "max(" + formatNumber(max, 3) + ")\n";
            }

            ImGui::Text(text.c_str());
        }

        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::drawBox(glm::dvec3 position, glm::dvec3 scale, glm::ivec3 color)
{

    Box box;
    box.min = position - scale / 2.0;
    box.max = position + scale / 2.0;
    ;
    box.color = color;

    drawQueue.boxes.push_back(box);

    //_drawBox(camera.get(), position, scale, color);
}

void Renderer::drawBoundingBox(glm::dvec3 position, glm::dvec3 scale, glm::ivec3 color)
{

    Box box;
    box.min = position - scale / 2.0;
    box.max = position + scale / 2.0;
    ;
    box.color = color;

    drawQueue.boundingBoxes.push_back(box);
}

void Renderer::drawBoundingBoxes(Camera *camera, GLBuffer buffer)
{
    _drawBoundingBoxesIndirect(camera, buffer);
}

void Renderer::drawPoints(void *points, int numPoints)
{
    _drawPoints(camera.get(), points, numPoints);
}

void Renderer::drawPoints(GLuint vao, GLuint vbo, int numPoints)
{
    _drawPoints(camera.get(), vao, vbo, numPoints);
}
