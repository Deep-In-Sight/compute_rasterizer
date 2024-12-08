#include <GLTimerQueries.h>
#include <ImGuiOverlay.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <unsuck.hpp>

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

void ImGuiInit(GLFWwindow *window)
{ // SETUP IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    ImGui::StyleColorsDark();
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

void ImGuiDestroy()
{
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}