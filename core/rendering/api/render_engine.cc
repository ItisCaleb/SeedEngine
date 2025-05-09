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
#include <spdlog/spdlog.h>

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }

template <>
RenderEngine::RenderEngine(GLFWwindow *window, int w, int h) {
    instance = this;
    spdlog::info("Initializing Rendering engine");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("Can't initialize GLAD. Exiting");
        exit(1);
    }

    this->device = new RenderDeviceOpenGL();
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    this->renderers.push_back(new ModelRenderer);
    this->renderers.push_back(new TerrainRenderer);
    for (Renderer *rd : this->renderers) {
        rd->init();
    }
    matrices_rc.alloc_constant("Matrices", sizeof(Mat4) * 3, NULL);
    cam_rc.alloc_constant("Camera", sizeof(Vec3), NULL);
    u8 white_tex[4] = {255, 255, 255, 255};
    default_tex.create(1, 1, (const char *)white_tex);
    default_mat.create();
    for (u32 i = 0; i < Material::MAX; i++) {
        default_mat->set_texture_map(static_cast<Material::TextureMapType>(i),
                                     default_tex);
    }
}

RenderDevice *RenderEngine::get_device() { return device; }

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }

Camera *RenderEngine::get_cam() { return &cam; }

void RenderEngine::process() {
    RenderCommandDispatcher dp(0);

    Mat4 *matrices = (Mat4 *)dp.map_buffer(&matrices_rc, 0, sizeof(Mat4) * 2);
    matrices[0] = cam.perspective().transpose();
    matrices[1] = cam.look_at().transpose();
    Vec3 *cam_pos = (Vec3 *)dp.map_buffer(&cam_rc, 0, sizeof(Vec3));
    *cam_pos = this->cam.get_position();
    u8 layer = 1;
    for (Renderer *rd : this->renderers) {
        rd->preprocess();
        rd->process(layer++);
    }
    this->device->process();
    for (Renderer *rd : this->renderers) {
        rd->cleanup();
    }
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed