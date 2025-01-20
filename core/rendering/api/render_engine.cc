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
    u8 gray_tex[4] = {200, 200, 200, 255};
    default_texture.alloc_texture(1, 1, gray_tex);

    RenderResource::register_resource("Matrices", matrices_rc);
    RenderResource::register_resource("Lights", lights_rc);
    RenderResource::register_resource("Material", mat_rc);
    RenderResource::register_resource("Camera", cam_rc);
}

RenderDevice *RenderEngine::get_device() { return device; }
void RenderEngine::register_mesh(Mesh *mesh) {
    this->mesh_instances[mesh] = {};
}

void RenderEngine::remove_mesh(Mesh *mesh) { this->mesh_instances.erase(mesh); }

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }

void RenderEngine::process() {
    std::vector<ModelEntity *> &entities =
        SeedEngine::get_instance()->get_world()->get_model_entities();
    for (ModelEntity *e : entities) {
        Ref<Mesh> mesh = e->get_mesh();
        if (mesh.is_null()) continue;
        Ref<Material> mat = e->get_material();
        auto &instance = mesh_instances[*mesh];

        instance[mat.ptr()].push_back(e->get_transform().transpose());
    }

    RenderCommandDispatcher dp;
    /* color pass */
    for (auto &[mesh, instances] : mesh_instances) {
        for (auto &e : instances) {
            Material *material = e.first;
            std::vector<Mat4> &models = e.second;
            dp.begin();
            dp.use(&mesh->vertices_rc);
            dp.use(&mesh->vertices_desc_rc);
            dp.use(&mesh->indices_rc);
            dp.update(&mesh->instance_rc, 0, sizeof(Mat4) * models.size(),
                      (void *)models.data());
            dp.use(&mesh->instance_rc);
            dp.use(&mesh->instance_desc_rc);

            if (material == nullptr || !material->texture_rc.inited()) {
                dp.use(&default_texture, 0);
            } else {
                dp.use(&material->texture_rc, 0);
            }
            if (material == nullptr || !material->diffuse_map.inited()) {
                dp.use(&default_texture, 1);
            } else {
                dp.use(&material->diffuse_map, 1);
            }
            if (material == nullptr || !material->specular_map.inited()) {
                dp.use(&default_texture, 2);
            } else {
                dp.use(&material->specular_map, 2);
            }

            dp.render(this->default_shader, models.size());
            dp.end();
        }
    }

    for (auto &[mesh, instances] : mesh_instances) {
        for (auto &models : instances) {
            models.second.clear();
        }
    }

    this->device->process();
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed