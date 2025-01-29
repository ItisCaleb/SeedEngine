#include "render_engine.h"
#include "core/engine.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/rendering/material.h"
#include "render_device_opengl.h"
#include <spdlog/spdlog.h>

namespace Seed {
RenderEngine *RenderEngine::get_instance() { return instance; }
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
    color_pass.init();
    debug_pass.init();

    RenderResource matrices_rc, mat_rc, cam_rc;

    matrices_rc.alloc_constant(sizeof(Mat4) * 3, NULL);
    mat_rc.alloc_constant(sizeof(Material), NULL);
    cam_rc.alloc_constant(sizeof(Vec3), NULL);

    RenderResource::register_resource("Matrices", matrices_rc);
    RenderResource::register_resource("Material", mat_rc);
    RenderResource::register_resource("Camera", cam_rc);
}

RenderDevice *RenderEngine::get_device() { return device; }

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }


void RenderEngine::process() {
    std::vector<ModelEntity *> &entities =
        SeedEngine::get_instance()->get_world()->get_model_entities();
    for (ModelEntity *e : entities) {
        Ref<Model> model = e->get_model();
        if (model.is_null()) continue;
        u32 variant = e->get_material_variant();
        auto &instance =  color_pass.model_instances[*model][variant];
        instance.push_back(e->get_transform().transpose());
        debug_pass.entity_aabb.push_back(e->get_model_aabb());
    }

    RenderCommandDispatcher dp;
    /* color pass */
    color_pass.process(dp, 0);
    debug_pass.process(dp, 0x10000000);
    this->device->process();
    for (auto &[model, instances] : color_pass.model_instances) {
        for (auto &[_, matrices] : instances) {
            matrices.clear();
        }
    }

    debug_pass.entity_aabb.clear();


    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed