#include "render_engine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/resource/material.h"
#include "opengl_backend.h"
#include "core/rendering/renderer/model_renderer.h"
#include "core/rendering/renderer/imgui_renderer.h"
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
    this->current_window = window;
    matrices_rc.alloc_constant("Matrices", sizeof(Mat4) * 3, NULL);
    cam_rc.alloc_constant("Camera", sizeof(Vec3), NULL);
    // u8 white_tex[4] = {255, 255, 255, 255};
    // default_tex.create(TextureType::TEXTURE_2D, 1, 1, (const u8 *)white_tex);
    // default_mat.create();
    // for (u32 i = 0; i < Material::MAX; i++) {
    //     default_mat->set_texture_map(static_cast<Material::TextureMapType>(i),
    //                                  default_tex);
    // }
}

void RenderEngine::init() {
    u32 i = 1;
    this->register_renderer<ModelRenderer>(i++);
    this->register_renderer<ImguiRenderer>(i++);
}

RenderBackend *RenderEngine::get_device() { return device; }

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }

Camera *RenderEngine::get_cam() { return &cam; }

template <typename T, typename... Args>
void RenderEngine::register_renderer(u32 layer, const Args &...args) {
    static_assert(std::is_base_of<Renderer, T>::value,
                  "T must be a derived class of Renderer.");
    Renderer *renderer = static_cast<Renderer *>(new T(args...));
    this->layers.push_back(Layer(this->current_window, renderer));
    renderer->set_layer(layer);
    renderer->init();
}

void RenderEngine::set_layer_viewport(u32 layer, RectF rect) {
    EXPECT_INDEX_INBOUND(layer - 1, this->layers.size());
    this->layers[layer - 1].viewport.set_dimension(rect);
}

Viewport &RenderEngine::get_layer_viewport(u32 layer) {
    EXPECT_INDEX_INBOUND_THROW(layer - 1, this->layers.size());
    return this->layers[layer - 1].viewport;
}

void RenderEngine::process() {
    RenderCommandDispatcher dp(0);
    {
        RenderStateDataBuilder builder;
        builder.clear(StateClearFlag::CLEAR_COLOR);
        builder.clear(StateClearFlag::CLEAR_DEPTH);
        dp.set_states(builder, 0);
    }
    Mat4 *matrices = (Mat4 *)dp.map_buffer(matrices_rc, 0, sizeof(Mat4) * 2);
    matrices[0] = cam.perspective().transpose();
    matrices[1] = cam.look_at().transpose();
    Vec3 *cam_pos = (Vec3 *)dp.map_buffer(cam_rc, 0, sizeof(Vec3));
    *cam_pos = this->cam.get_position();
    u32 i = 1;
    for (Layer &layer : this->layers) {
        RenderCommandDispatcher layer_dp(i++);
        Rect rect = layer.viewport.get_actual_dimension();
        {
            RenderStateDataBuilder builder;
            builder.set_viewport(rect.x, rect.y, rect.w, rect.h);
            layer_dp.set_states(builder, 0);
        }
        layer.renderer->preprocess();
        layer.renderer->process(layer.viewport);
    }
    this->device->process();
    for (Layer &layer : this->layers) {
        layer.renderer->cleanup();
    }
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed