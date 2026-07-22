#include "engine.h"
#include "core/system.h"
#include <GLFW/glfw3.h>
#include "core/io/file.h"
#include "core/project.h"
#include "core/window.h"
#include "core/world/world.h"
#include "debug/profiler.h"
#include "input.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_loader.h"
#include "core/gui/gui_engine.h"
#include "core/concurrency/thread_pool.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "input_handler.h"
#include "types.h"
#include <spdlog/spdlog.h>
#include <stdlib.h>
#include "core/os.h"
#include "core/debug/debug_drawer.h"
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Seed {

namespace System {
SeedEngine *gEngine = nullptr;
RenderEngine *gRenderEngine = nullptr;
ResourceEntries *gResourceEntries = nullptr;
ResourceLoader *gResourceLoader = nullptr;
GuiEngine *gGuiEngine = nullptr;
Input *gInput = nullptr;
DefaultStorage *gDefaultStorage = nullptr;
DebugDrawer *gDebugDrawer = nullptr;
ThreadPool *gThreadPool = nullptr;
PhysicEngine *gPhysicEngine = nullptr;
Profiler *gProfiler = nullptr;
};  // namespace System

static void error_callback(int error, const char *description) {
    spdlog::error("GLFW Error: {}", description);
}

void SeedEngine::setup_logger() {
    auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
        [](const spdlog::details::log_msg &msg) { return; });
    callback_sink->set_level(spdlog::level::err);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::logger logger("Main", {console_sink, callback_sink});
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
}

void SeedEngine::init_systems() {
    System::gResourceLoader = new ResourceLoader;
    System::gResourceEntries = new ResourceEntries;
#ifdef SEED_XR
    XREngine *xr_engine = new XREngine();
#endif
    System::gEngine = this;
    System::gRenderEngine = new RenderEngine(window);
    System::gGuiEngine = new GuiEngine(this->window);
    input_handler.init(this->window);
    System::gInput = new Input;
    System::gDefaultStorage = new DefaultStorage();
    System::gDebugDrawer = new DebugDrawer();
    System::gThreadPool = new ThreadPool(OS::cpu_count());
    System::gPhysicEngine = new PhysicEngine();
    System::gProfiler = new Profiler;
    System::gRenderEngine->init();
    this->world = new World;
}

void SeedEngine::deinit_systems() {
    delete System::gProfiler;
    delete System::gPhysicEngine;
    delete System::gThreadPool;
    delete System::gDebugDrawer;
    delete System::gDefaultStorage;
    delete System::gInput;
    delete System::gGuiEngine;
    delete System::gRenderEngine;
    delete System::gResourceEntries;
    delete System::gResourceLoader;
}

bool SeedEngine::load_project(const Path &path) {
    current_project = Project::load(path);
    if (!File::exists(current_project->get_entry_path())) {
        System::gResourceEntries->save(current_project->get_entry_path());
    } else {
        System::gResourceEntries->load(current_project->get_entry_path());
    }
    return current_project != nullptr;
}

void SeedEngine::start() {
    if (window == nullptr) {
        return;
    }
    spdlog::info("Starting SeedEngine");
    f64 delta = frame_limit;
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();
    while (!glfwWindowShouldClose(glfw_window)) {
        f64 start = glfwGetTime();

        input_handler.update();
        glfwPollEvents();
        System::gGuiEngine->update();
        world->tick(delta);

        System::gRenderEngine->process();

        delta = glfwGetTime() - start;
        if (delta < frame_limit) {
            OS::delay(frame_limit - delta);
            delta = frame_limit;
        }
        System::gProfiler->clear_records();
        last_fps = 1 / delta;
    }
}

SeedEngine::SeedEngine(f32 target_fps) {
    setup_logger();
    glfwSetErrorCallback(error_callback);
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Initializing SeedEngine");

    if (!glfwInit()) {
        spdlog::error("Can't initialize GLFW. Exiting");
        exit(1);
    }

    window = new Window(1960, 1024, "Ave Mujica");
    init_systems();

    this->frame_limit = 1 / target_fps;
}
SeedEngine::~SeedEngine() {
    deinit_systems();
    delete window;
    glfwTerminate();
}
}  // namespace Seed
