#include "render_engine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/resource/material.h"
#include "opengl_backend.h"
#include "core/rendering/renderer/default_renderer.h"
#include "core/rendering/renderer/imgui_renderer.h"
#include "core/rendering/renderer/post_renderer.h"
#include "core/macro.h"

#include <spdlog/spdlog.h>

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }

RenderEngine::RenderEngine(Window *window) {
    instance = this;
    spdlog::info("Initializing Rendering engine");
    if (!window) {
        SPDLOG_ERROR(
            "Can't initialize Render engine, window is null, exiting.");
        exit(1);
    }
    GLFWwindow *glfw_window = window->get_window<GLFWwindow>();

    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("Can't initialize GLAD. Exiting");
        exit(1);
    }
    this->device = new RenderBackendGL;
    this->mesh_storage = new MeshStorage;
    this->current_window = window;
    this->instance_pools["TransformDataPool"] =
        new InstanceDataPool(sizeof(Mat4), 65536);
    this->instance_pools["TerrainDataPool"] =
        new InstanceDataPool(sizeof(Vec4), 1024);
    RenderCommandDispatcher dp;
    u32 i = 0;
    for (auto &[name, pool] : instance_pools) {
        RenderStateDataBuilder builder;
        builder.bind_bufferbase(pool->get_render_buffer(), i++);
        dp.set_states(builder, 0);
    }
    matrices_rc.alloc_constant("Matrices", sizeof(Mat4) * 3, NULL);
    cam_rc.alloc_constant("Camera", sizeof(Vec3), NULL);
}

void RenderEngine::init() {
    u32 i = 1;
    Ref<WindowRenderTarget> window_rt;
    Ref<MultiRenderTarget> post_target;
    window_rt.create();
    post_target.create();
    this->render_targets["default"] = ref_cast<RenderTarget>(post_target);
    this->render_targets["window"] = ref_cast<RenderTarget>(window_rt);

    Ref<Texture> color_tex(TextureType::TEXTURE_2D, 1024, 768,
                           PixelFormat::RGBA16F, nullptr);
    Ref<Texture> depth_tex(TextureType::TEXTURE_2D, 1024, 768,
                           PixelFormat::D24S8, nullptr);
    post_target->bind_color(0, color_tex);
    post_target->bind_depth(depth_tex);

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
    this->layers.push_back(Layer(rt, current_window, renderer));
    renderer->set_layer(layer);
    renderer->init();
}

void RenderEngine::set_layer_viewport(u32 layer, RectF rect) {
    EXPECT_INDEX_INBOUND(layer - 1, this->layers.size());
    this->layers[layer - 1].vp.set_dimension(rect);
}

Viewport *RenderEngine::get_layer_viewport(u32 layer) {
    EXPECT_INDEX_INBOUND_THROW(layer - 1, this->layers.size());
    return &this->layers[layer - 1].vp;
}

void RenderEngine::process() {
    RenderCommandDispatcher dp;
    for (auto &iter : this->render_targets) {
        RenderStateDataBuilder builder;
        builder.bind_render_target(iter.second->get_resource());
        builder.clear(StateClearFlag::CLEAR_COLOR);
        builder.clear(StateClearFlag::CLEAR_DEPTH);
        dp.set_states(builder, 0);
    }
    RenderUpdateData *upd = dp.map_buffer(matrices_rc, 0, sizeof(Mat4) * 2);
    Mat4 *matrices = (Mat4 *)upd->get_buffer();
    matrices[0] = cam.projection().transpose();
    matrices[1] = cam.look_at().transpose();
    upd->set_filled();
    upd = dp.map_buffer(cam_rc, 0, sizeof(Vec3));
    Vec3 *cam_pos = (Vec3 *)upd->get_buffer();
    *cam_pos = this->cam.get_position();
    upd->set_filled();
    for (Layer &layer : this->layers) {
        RenderCommandDispatcher layer_dp;
        {
            RenderStateDataBuilder builder;
            builder.set_viewport(&layer.vp);
            builder.bind_render_target(layer.rt->get_resource());
            layer_dp.set_states(builder, layer.renderer->current_sort_key());
        }
        layer.renderer->preprocess();
        layer.renderer->process(layer.vp);
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

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed