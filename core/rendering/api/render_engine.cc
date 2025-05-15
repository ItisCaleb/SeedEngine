#include "render_engine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/resource/material.h"
#include "render_device_opengl.h"
#include "core/rendering/renderer/model_renderer.h"
#include "core/rendering/renderer/terrain_renderer.h"
#include "core/rendering/renderer/imgui_renderer.h"

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
    this->device = new RenderDeviceOpenGL();
    u8 layer = 1;
    this->register_renderer<ModelRenderer>(layer++);
    this->register_renderer<TerrainRenderer>(layer++);
    this->register_renderer<ImguiRenderer>(layer++);

    
    matrices_rc.alloc_constant("Matrices", sizeof(Mat4) * 3, NULL);
    cam_rc.alloc_constant("Camera", sizeof(Vec3), NULL);
    u8 white_tex[4] = {255, 255, 255, 255};
    default_tex.create(TextureType::TEXTURE_2D, 1, 1, (const u8 *)white_tex);
    default_mat.create();
    for (u32 i = 0; i < Material::MAX; i++) {
        default_mat->set_texture_map(static_cast<Material::TextureMapType>(i),
                                     default_tex);
    }
}

RenderDevice *RenderEngine::get_device() { return device; }

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }

Camera *RenderEngine::get_cam() { return &cam; }

template<typename T, typename ...Args>
void RenderEngine::register_renderer(const Args &...args){
    static_assert(std::is_base_of<Renderer, T>::value,
        "T must be a derived class of Renderer.");
    Renderer* renderer = static_cast<Renderer*>(new T(args...));
    this->renderers.push_back(renderer);
    renderer->init();
}

void RenderEngine::process() {
    RenderCommandDispatcher dp(0);
    dp.clear(StateClearFlag::CLEAR_COLOR);
    dp.clear(StateClearFlag::CLEAR_DEPTH);
    Mat4 *matrices = (Mat4 *)dp.map_buffer(matrices_rc, 0, sizeof(Mat4) * 2);
    matrices[0] = cam.perspective().transpose();
    matrices[1] = cam.look_at().transpose();
    Vec3 *cam_pos = (Vec3 *)dp.map_buffer(cam_rc, 0, sizeof(Vec3));
    *cam_pos = this->cam.get_position();
    for (Renderer *rd : this->renderers) {
        rd->preprocess();
        rd->process();
    }
    this->device->process();
    for (Renderer *rd : this->renderers) {
        rd->cleanup();
    }
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed