#include "engine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "input.h"
#include "core/rendering/api/render_engine.h"
#include "core/resource/resource_loader.h"
#include "core/gui/gui_engine.h"
#include "core/concurrency/thread_pool.h"
#include "core/resource/default_storage.h"
#include "types.h"
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/os.h"
#include "core/debug/debug_drawer.h"
namespace Seed {

static void error_callback(int error, const char *description) {
    spdlog::error("GLFW Error: {}", description);
}

SeedEngine *SeedEngine::get_instance() { return instance; }

void SeedEngine::init_systems() {
    ResourceLoader *resource_loader = new ResourceLoader;
    Input *input = new Input;
    input_handler.init(this->window);
    GuiEngine *gui = new GuiEngine(this->window);
    RenderEngine *render_engine = new RenderEngine(window);
    DefaultStorage *storage = new DefaultStorage();
    DebugDrawer *debug_drawer = new DebugDrawer();
    ThreadPool *pool = new ThreadPool(OS::cpu_count());
    render_engine->init();
    this->world = new World;
}

void SeedEngine::start() {
    if (window == nullptr) {
        return;
    }
    spdlog::info("Starting SeedEngine");
    Input *input = Input::get_instance();
    RenderEngine *render_engine = RenderEngine::get_instance();
    f64 delta = frame_limit;
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();
    while (!glfwWindowShouldClose(glfw_window)) {
        f64 start = glfwGetTime();
        if (input->is_key_pressed(KeyCode::Q)) {
            break;
        }

        glfwPollEvents();
        GuiEngine::get_instance()->update();
        world->tick(delta);

        render_engine->process();
        glfwSwapBuffers(glfw_window);

        delta = glfwGetTime() - start;

        if (delta < frame_limit) {
            OS::delay(frame_limit - delta);
            delta = frame_limit;
        }
    }

    glfwDestroyWindow(glfw_window);

    glfwTerminate();
}

SeedEngine::SeedEngine(f32 target_fps) {
    instance = this;

    glfwSetErrorCallback(error_callback);
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Initializing SeedEngine");

    if (!glfwInit()) {
        spdlog::error("Can't initialize GLFW. Exiting");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    window = new Window(1260, 768, "Ave Mujica");
    if (!window) {
        spdlog::error("Can't create window. Exiting");
        glfwTerminate();
        exit(1);
    }

    init_systems();

    this->frame_limit = 1 / target_fps;
}
SeedEngine::~SeedEngine() { instance = nullptr; }
}  // namespace Seed