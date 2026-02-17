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
#include "core/rendering/renderer/post_renderer.h"
#include "core/macro.h"

#include <spdlog/spdlog.h>

namespace Seed {
inline Mat4 RenderEngine::get_window_projection() {
    f32 L = 0;
    f32 R = current_window->get_width();
    f32 T = 0;
    f32 B = current_window->get_height();
    const Mat4 ortho_projection = {
        Vec4{2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        Vec4{0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        Vec4{0.0f, 0.0f, -1.0f, 0.0f},
        Vec4{(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    return ortho_projection;
}

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
    visible_ssbo = RHI::alloc_storage_buffer(sizeof(int) * 65536,
                                             UpdateFrequence::PERFRAME, nullptr);
    cam_rc =
        RHI::alloc_constant(sizeof(Vec3), UpdateFrequence::PERFRAME, nullptr);
    matrices_rc = RHI::alloc_constant(sizeof(Mat4) * 3,
                                      UpdateFrequence::PERFRAME, nullptr);
    RenderCommandDispatcher dp;
    /* Bind engine default buffers */
    RenderStateDataBuilder builder;
    builder.bind_storage_buffer(visible_ssbo, 0);
    builder.bind_storage_buffer(
        this->instance_pools["TransformDataPool"]->get_render_buffer(), 1);
    builder.bind_storage_buffer(
        this->instance_pools["TerrainDataPool"]->get_render_buffer(), 2);
    builder.bind_storage_buffer(
        this->instance_pools["BonesPool"]->get_render_buffer(), 3);
    builder.bind_constant(cam_rc, 8);
    builder.bind_constant(matrices_rc, 9);
    dp.set_states(builder, 0);
}

void RenderEngine::init() {
    u32 i = 1;
    Ref<WindowRenderTarget> window_rt;
    Ref<MultiRenderTarget> post_target;

    u32 res_w = current_window->get_width() * 2;
    u32 res_h = current_window->get_height() * 2;

    Ref<Texture> color_tex(TextureType::TEXTURE_2D, res_w, res_h,
                           PixelFormat::RGBA16F, nullptr);
    Ref<Texture> depth_tex(
        TextureType::TEXTURE_2D, res_w, res_h, PixelFormat::D24S8, nullptr,
        SamplerProperty{.min_filter = SamplerFilter::NEAREST,
                        .mag_filter = SamplerFilter::NEAREST});

    window_rt.create(current_window);
    post_target.create(Viewport(Vec2{(f32)res_w, (f32)res_h}));
    post_target->bind_color(0, color_tex);
    post_target->bind_depth(depth_tex);
    this->render_targets["default"] = ref_cast<RenderTarget>(post_target);
    this->render_targets["window"] = ref_cast<RenderTarget>(window_rt);

    this->register_renderer<DefaultRenderer>(
        i++, ref_cast<RenderTarget>(post_target));
    this->register_renderer<PostRenderer>(i++,
                                          ref_cast<RenderTarget>(window_rt));
    this->register_renderer<ImguiRenderer>(i++,
                                           ref_cast<RenderTarget>(window_rt));
}

RenderBackend *RenderEngine::get_device() { return device; }

Camera *RenderEngine::get_cam() { return &cam; }

template <typename T, typename... Args>
void RenderEngine::register_renderer(u32 layer, Ref<RenderTarget> rt,
                                     const Args &...args) {
    static_assert(std::is_base_of<Renderer, T>::value,
                  "T must be a derived class of Renderer.");
    Renderer *renderer = static_cast<Renderer *>(new T(args...));
    this->layers.push_back(Layer(rt, renderer));
    renderer->set_layer(layer);
    renderer->init();
}

void RenderEngine::process() {
    RenderCommandDispatcher dp;
    for (auto &iter : this->render_targets) {
        RenderStateDataBuilder builder;
        builder.bind_render_target(iter.second->get_handle());
        builder.set_scissor(iter.second->get_viewport());
        builder.set_viewport(iter.second->get_viewport());
        builder.clear(StateClearFlag::CLEAR_COLOR);
        builder.clear(StateClearFlag::CLEAR_DEPTH);
        dp.set_states(builder, 0);
    }
    Mat4 *matrices = (Mat4 *)RHI::alloc_heap(sizeof(Mat4) * 3);
    Vec3 *cam_pos = (Vec3 *)RHI::alloc_heap(sizeof(Vec3));
    matrices[0] = cam.projection_zero();
    matrices[1] = cam.look_at();
    matrices[2] = get_window_projection().transpose();
    *cam_pos = this->cam.get_position();
    RHI::update_from_heap(matrices_rc, 0, sizeof(Mat4) * 3, matrices);
    RHI::update_from_heap(cam_rc, 0, sizeof(Vec3), cam_pos);
    for (Layer &layer : this->layers) {
        RenderCommandDispatcher layer_dp;
        {
            RenderStateDataBuilder builder;
            builder.set_viewport(layer.rt->get_viewport());
            builder.bind_render_target(layer.rt->get_handle());
            layer_dp.set_states(builder, layer.renderer->current_sort_key());
        }
        layer.renderer->preprocess();
        layer.renderer->process(*layer.rt->get_viewport());
    }

    this->device->process();
    for (Layer &layer : this->layers) {
        layer.renderer->cleanup();
    }
}

Ref<RenderTarget> RenderEngine::get_render_target(const std::string &name) {
    auto iter = this->render_targets.find(name);
    if (iter != this->render_targets.end()) {
        return iter->second;
    }
    return Ref<RenderTarget>();
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