

#include "Renderer.h"
#include "Runtime.h"
#include <OrbitControls.h>
#include <cuda.h>
#include <iostream>

#include "compute/LasLoaderSparse.h"
#include "compute_loop_las/compute_loop_las.h"

using namespace std;

int numPoints = 1'000'000;

int main()
{

    cout << std::setprecision(2) << std::fixed;

    auto renderer = Renderer::Instance();
    // renderer->init();

    // Creating a CUDA context
    cuInit(0);
    CUdevice cuDevice;
    CUcontext context;
    cuDeviceGet(&cuDevice, 0);
    cuCtxCreate(&context, 0, cuDevice);

    auto tStart = now();

    renderer->controls->yaw = 0.53;
    renderer->controls->pitch = -0.68;
    renderer->controls->radius = 2310.47;
    renderer->controls->target = {576.91, 886.62, 10.35};

    auto lasLoaderSparse = make_shared<LasLoaderSparse>(renderer);

    Runtime::lasLoaderSparse = lasLoaderSparse;

    renderer->addFileDropCallback([lasLoaderSparse, renderer](vector<string> files) {
        vector<string> lasfiles;

        for (auto file : files)
        {
            if (iEndsWith(file, "las") || iEndsWith(file, "laz"))
            {
                lasfiles.push_back(file);
            }
        }

        lasLoaderSparse->add(lasfiles, [renderer](vector<shared_ptr<LasFile>> lasfiles) {
            dvec3 boxMin = {Infinity, Infinity, Infinity};
            dvec3 boxMax = {-Infinity, -Infinity, -Infinity};

            for (auto lasfile : lasfiles)
            {
                boxMin = glm::min(boxMin, lasfile->boxMin);
                boxMax = glm::max(boxMax, lasfile->boxMax);
            }

            // zoom to point cloud
            auto size = boxMax - boxMin;
            auto position = (boxMax + boxMin) / 2.0;
            auto radius = glm::length(size) / 1.5;

            renderer->controls->yaw = 0.53;
            renderer->controls->pitch = -0.68;
            renderer->controls->radius = radius;
            renderer->controls->target = position;
        });

        glfwFocusWindow(renderer->window);
    });

    { // 4-4-4 byte format
        auto computeLoopLas = new ComputeLoopLas(renderer, lasLoaderSparse);
        Runtime::addMethod(computeLoopLas);
        Runtime::setSelectedMethod("loop_las");
    }

    auto update = [&]() {
        lasLoaderSparse->process();

        auto selected = Runtime::getSelectedMethod();
        if (selected)
        {
            selected->update(renderer);
        }
    };

    auto render = [&]() {
        {

            auto &view = renderer->views[0];

            view.view = renderer->camera->view;
            view.proj = renderer->camera->proj;

            renderer->views[0].framebuffer->setSize(renderer->width, renderer->height);
        }

        {
            auto selected = Runtime::getSelectedMethod();
            if (selected)
            {
                selected->render(renderer);
            }
        }
    };

    renderer->loop(update, render);

    return 0;
}
