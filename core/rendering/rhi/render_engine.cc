#include "render_engine.h"
#include <GLFW/glfw3.h>
#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/resource/material.h"
#include "core/rendering/backend/vulkan_backend.h"
#include "core/rendering/renderer/default_renderer.h"
#include "core/rendering/renderer/imgui_renderer.h"
#include "core/macro.h"

#include <spdlog/spdlog.h>

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }

void RenderEngine::bind_opengl(Window *window) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    window->create();

    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);

    // this->device = new RenderBackendGL;
}

void RenderEngine::bind_vulken(Window *window) {
    spdlog::info("Initializing Vulkan Rendering backend");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->create();

    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    this->device = new RenderBackendVK(window);
}

static const std::vector<std::string> DEFAULT_INCLUDE_PATHS = {
    "assets/shader",
};

RenderEngine::RenderEngine(Window *window) {
    instance = this;
    spdlog::info("Initializing Rendering engine");
    if (!window) {
        SPDLOG_ERROR(
            "Can't initialize Render engine, window is null, exiting.");
        exit(1);
    }
    bind_vulken(window);

    this->shader_proxy = new ShaderProxy(DEFAULT_INCLUDE_PATHS);
    this->mesh_storage = new MeshStorage;
    this->current_window = window;
    this->instance_pools["TransformDataPool"] =
        new InstanceDataPool(sizeof(Mat4), 65536);
    this->instance_pools["TerrainDataPool"] =
        new InstanceDataPool(sizeof(Vec4), 1024);
    this->instance_pools["BonesPool"] =
        new InstanceDataPool(sizeof(Vec4), 1024);
}

void RenderEngine::init() {
    u32 i = 1;
    this->register_renderer<DefaultRenderer>(i++);
    this->register_renderer<ImguiRenderer>(i++);
}

RenderBackend *RenderEngine::get_device() { return device; }

template <typename T, typename... Args>
void RenderEngine::register_renderer(u32 layer, const Args &...args) {
    static_assert(std::is_base_of<Renderer, T>::value,
                  "T must be a derived class of Renderer.");
    Renderer *renderer =
        static_cast<Renderer *>(new T(args...));
    this->renderers.push_back(renderer);
    renderer->set_layer(layer);
    renderer->init(current_window);
}

void RenderEngine::process() {
    for (Renderer *rd : this->renderers) {
        rd->preprocess();
        rd->process();
    }

    this->device->process();
    for (Renderer *rd : this->renderers) {
        rd->cleanup();
    }
}

InstanceDataPool *RenderEngine::get_instance_pool(const std::string &name) {
    auto iter = this->instance_pools.find(name);
    if (iter != this->instance_pools.end()) {
        return iter->second;
    }
    return nullptr;
}

ShaderHandle RenderEngine::compile_shader(const std::string &path,
                                          const std::string &shader,
                                          ShaderLayout *layout) {
    return this->shader_proxy->compile_shader(path, shader, layout);
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed