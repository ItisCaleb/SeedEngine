#include <seed/engine.h>
#include <seed/input.h>
#include <seed/render_engine.h>
#include <seed/resource.h>
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

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
    KeyCode k = static_cast<KeyCode>(key);
    if (action == GLFW_PRESS) {
        Input::get_instance()->press_key(k);
    } else if (action == GLFW_RELEASE) {
        Input::get_instance()->release_key(k);
    }
}
SeedEngine *SeedEngine::get_instance() { return instance; }

void SeedEngine::delay(f32 seconds) {
#ifdef _WIN32
    Sleep((u32)(seconds * 1000));
#else
    usleep((u32)(seconds * 1000000));
#endif
}

void SeedEngine::init_systems() {
    Input *input = new Input;
    RenderEngine *render_engine = new RenderEngine(window, width, height);
    ResourceLoader *resource_loader = new ResourceLoader;
}

void SeedEngine::start() {
    if (window == nullptr) {
        return;
    }
    spdlog::info("Starting SeedEngine");
    Input *input = Input::get_instance();
    while (!glfwWindowShouldClose(window)) {
        f64 start = glfwGetTime();
        if (input->is_key_pressed(KeyCode::Q)) {
            break;
        }
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
    glfwSetErrorCallback(error_callback);

    spdlog::info("Initializing SeedEngine");

    if (!glfwInit()) {
        spdlog::error("Can't initialize GLFW. Exiting");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    width = 640;
    height = 480;
    window = glfwCreateWindow(width, height, "Simple example", NULL, NULL);
    if (!window) {
        spdlog::error("Can't create window. Exiting");
        glfwTerminate();
        exit(1);
    }
    glfwSetKeyCallback(window, key_callback);

    init_systems();

    this->frame_limit = 1 / target_fps;
}
SeedEngine::~SeedEngine() { instance = nullptr; }
}  // namespace Seed