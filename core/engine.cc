#include "engine.h"
#include <GLFW/glfw3.h>
#include "input.h"
#include "core/rendering/api/render_engine.h"
#include "core/resource/resource_loader.h"
#include "core/gui/gui_engine.h"
#include "core/concurrency/thread_pool.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
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
    RenderEngine *render_engine = new RenderEngine(window);
    Input *input = new Input;
    input_handler.init(this->window);
    GuiEngine *gui = new GuiEngine(this->window);
    DefaultStorage *storage = new DefaultStorage();
    DebugDrawer *debug_drawer = new DebugDrawer();
    ThreadPool *pool = new ThreadPool(OS::cpu_count());
    PhysicEngine *phys_engine = new PhysicEngine();
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
        PhysicEngine::get_instance()->process();
        world->tick(delta);

        render_engine->process();

        delta = glfwGetTime() - start;
        if (delta < frame_limit) {
            OS::delay(frame_limit - delta);
            delta = frame_limit;
        }
        last_fps = 1 / delta;
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

    window = new Window(1260, 768, "Ave Mujica");
    init_systems();

    this->frame_limit = 1 / target_fps;
}
SeedEngine::~SeedEngine() { instance = nullptr; }
}  // namespace Seed