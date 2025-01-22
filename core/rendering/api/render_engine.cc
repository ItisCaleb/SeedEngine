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

    ResourceLoader *loader = ResourceLoader::get_instance();

    RenderResource shader_rc;
    try {
        shader_rc =
            loader->loadShader("assets/vertex.glsl", "assets/fragment.glsl");
    } catch (std::exception &e) {
        spdlog::error("Error loading Shader: {}", e.what());
        exit(1);
    }
    RenderResource::register_resource("Default", shader_rc);
    this->default_shader = &RenderResource::shaders["Default"];
    Lights lights;
    lights.ambient = Vec3{0.2, 0.2, 0.2};
    lights.lights[0].set_postion(Vec3{2, 0, 2});
    lights.lights[0].diffuse = Vec3{0.9, 0.5, 0.5};
    lights.lights[0].specular = Vec3{1, 1, 1};

    lights.lights[1].set_direction(Vec3{2, 3, -1});
    lights.lights[1].diffuse = Vec3{1, 1, 1};

    RenderResource matrices_rc, lights_rc, mat_rc, cam_rc;

    matrices_rc.alloc_constant(sizeof(Mat4) * 3, NULL);
    lights_rc.alloc_constant(sizeof(Lights), &lights);
    mat_rc.alloc_constant(sizeof(Material), NULL);
    cam_rc.alloc_constant(sizeof(Vec3), NULL);
    u8 white_tex[4] = {255, 255, 255, 255};
    RenderResource default_tex;
    default_tex.alloc_texture(1, 1, white_tex);
    default_material = Material::create(default_tex, default_tex, default_tex);

    RenderResource::register_resource("Matrices", matrices_rc);
    RenderResource::register_resource("Lights", lights_rc);
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
        auto &instance = model_instances[*model][variant];
        instance.push_back(e->get_transform().transpose());
    }

    RenderCommandDispatcher dp;
    u64 cnt = 0;
    /* color pass */
    for (auto &[model, instances] : model_instances) {
        dp.begin(cnt++);
        dp.use(&model->instance_rc);
        dp.use(&model->instance_desc_rc);

        for (auto &[mat_variant, matrices] : instances) {
            dp.begin(cnt++);
            dp.update(&model->instance_rc, 0, sizeof(Mat4) * matrices.size(),
                      (void *)matrices.data());
            for (Mesh &mesh : model->meshes) {
                dp.use(&mesh.vertices_rc);
                dp.use(&model->vertices_desc_rc);
                dp.use(&mesh.indices_rc);
                Ref<Material> material = model->model_mat.get_material(
                    mesh.material_handle, mat_variant);
                if(material.is_null()){
                    material = default_material;
                }
                if (material->diffuse_map.inited()) {
                    dp.use(&material->diffuse_map, 0);
                }else{
                    dp.use(&default_material->diffuse_map, 0);
                }
                if (material->specular_map.inited()) {
                    dp.use(&material->specular_map, 1);
                }else{
                    dp.use(&default_material->specular_map, 1);
                }
                dp.render(this->default_shader, matrices.size());
            }
            dp.end();
        }
        dp.end();
    }

    for (auto &[model, instances] : model_instances) {
        for (auto &[_, matrices] : instances) {
            matrices.clear();
        }
    }

    this->device->process();
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed