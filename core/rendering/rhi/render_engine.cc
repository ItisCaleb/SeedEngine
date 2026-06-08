#include "render_engine.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include "core/io/path.h"
#include "core/math/vec2.h"
#include "core/rendering/backend/vulkan_backend.h"
#include "core/rendering/renderer/default_renderer.h"
#include "core/rendering/renderer/imgui_renderer.h"
#include "core/debug/profiler.h"
#ifdef SEED_XR
#include "core/rendering/backend/xr_vulkan_backend.h"
#endif
#include "core/window.h"

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }

void RenderEngine::bind_vulken(Window *window) {
    spdlog::info("Initializing Vulkan Rendering backend");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->create();

    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    this->device = new RenderBackendVK(window);
}
void RenderEngine::bind_vulkan_xr(Window *window) {
#ifdef SEED_XR
    spdlog::info("Initializing Vulkan XR Rendering backend");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->create();

    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    this->device = new RenderBackendXRVk(window);
#else
    throw std::runtime_error("XR not supported!");
#endif
}

static const std::vector<std::string> DEFAULT_INCLUDE_PATHS = {
    "assets/shader",
};

void RenderEngine::set_renderer_layer(Renderer *renderer, u8 layer) {
    if (!renderer) {
        return;
    }
    for (RendererLayer &_layer : this->renderers) {
        if (_layer.renderer == renderer) {
            _layer.layer = layer;
            return;
        }
    }
}

void RenderEngine::set_renderer_enable(Renderer *renderer, bool enable) {
    if (!renderer) {
        return;
    }
    for (RendererLayer &_layer : this->renderers) {
        if (_layer.renderer == renderer) {
            _layer.enabled = false;
            return;
        }
    }
}

RenderEngine::RenderEngine(Window *window) {
    instance = this;
    spdlog::info("Initializing Rendering engine");
    if (!window) {
        SPDLOG_ERROR(
            "Can't initialize Render engine, window is null, exiting.");
        exit(1);
    }
#ifdef SEED_XR
    bind_vulkan_xr(window);
#else
    bind_vulken(window);
#endif
    this->shader_proxy = new ShaderProxy(DEFAULT_INCLUDE_PATHS);
    this->mesh_storage = new MeshStorage;
    this->current_window = window;
    this->instance_pools[TRANSFORM_POOL_NAME] =
        new InstanceDataPool(sizeof(Mat4), 65536);
    this->instance_pools[TERRAIN_POOL_NAME] =
        new InstanceDataPool(sizeof(Vec4), 1024);
    this->instance_pools[SKELETON_POOL_NAME] =
        new InstanceDataPool(sizeof(Mat4), 65536);
}

void RenderEngine::init() {
    default_renderer = new DefaultRenderer;
    imgui_renderer = new ImguiRenderer;
    this->register_renderer(0, default_renderer);
    this->register_renderer(10, imgui_renderer);
    g_frame.init();
}

RenderBackend *RenderEngine::get_device() { return device; }

void RenderEngine::register_renderer(u8 layer, Renderer *renderer) {
    this->renderers.push_back(
        RendererLayer{.layer = layer, .enabled = true, .renderer = renderer});
    renderer->init(current_window);
}

void RenderEngine::process() {
    PROFILE_SCOPE("Rendering");
    RenderCommandDispatcher dp;
    RenderStateDataBuilder builder;
    RenderEngine::get_instance()->get_frame_global().bind(builder);
    dp.set_states(builder);

    for (RendererLayer &layer : this->renderers) {
        if (layer.enabled) {
            layer.renderer->preprocess();
            layer.renderer->process(layer.layer);
        }
    }

    this->device->process();
    for (RendererLayer &layer : this->renderers) {
        if (layer.enabled) {
            layer.renderer->cleanup();
        }
    }
}

InstanceDataPool *RenderEngine::get_instance_pool(const std::string &name) {
    auto iter = this->instance_pools.find(name);
    if (iter != this->instance_pools.end()) {
        return iter->second;
    }
    return nullptr;
}

ShaderHandle RenderEngine::compile_shader(const Path &path,
                                          const std::string &shader,
                                          ShaderLayout *layout) {
    return this->shader_proxy->compile_shader(path, shader, layout);
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed