#include <GLFW/glfw3.h>
#include <seed/engine.h>
#include <seed/types.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace Seed {

static void error_callback(int error, const char *description) {
    spdlog::error("GLFW Error: {}", description);
}

SeedEngine *SeedEngine::get_instance() { return instance; }

void SeedEngine::delay(f32 seconds) {
#ifdef _WIN32
    Sleep((u32)(seconds * 1000));
#else
    usleep((u32)(seconds * 1000000));
#endif
}

void SeedEngine::start() {
    if (window == nullptr) {
        return;
    }
    spdlog::info("Starting SeedEngine");

    while (!glfwWindowShouldClose(window)) {
        f64 start = glfwGetTime();
        glfwPollEvents();

        glfwSwapBuffers(window);

        f64 delta = glfwGetTime() - start;
        if (delta < frame_limit) {
            delay(frame_limit - delta);
            delta = frame_limit;
        }
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}

SeedEngine::SeedEngine(f32 target_fps) {
    instance = this;
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);

    spdlog::info("Initializing SeedEngine");
    if (!glfwInit()) {
        spdlog::error("Can't initialize GLFW. Exiting");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window) {
        spdlog::error("Can't create window. Exiting");
        glfwTerminate();
        exit(1);
    }

    // glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    this->frame_limit = 1 / target_fps;
    this->window = window;
}
SeedEngine::~SeedEngine() { instance = nullptr; }
}  // namespace Seed