#include "render_engine.h"
#include "core/engine.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/rendering/material.h"
#include "render_device_opengl.h"
#include <spdlog/spdlog.h>

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam) {
    std::string source_str;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            break;
        default:
            return;
    }
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_str = "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "Source: Other";
            break;
    }
    spdlog::error("{}: {} ({})", source_str, message, userParam);
    return;
}

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
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);
    }
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

    ResourceLoader *loader = ResourceLoader::get_instance();

    RenderResource shader_rc;
    try {
        shader_rc =
            loader->loadShader("assets/vertex.glsl", "assets/fragment.glsl");
    } catch (std::exception &e) {
        spdlog::error("Error loading Shader: {}", e.what());
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
    Material mat = {0, 1, 2, 32.0f};
    mat_rc.alloc_constant(sizeof(Material), &mat);
    cam_rc.alloc_constant(sizeof(Vec3), NULL);

    RenderResource::register_resource("Matrices", matrices_rc);
    RenderResource::register_resource("Lights", lights_rc);
    RenderResource::register_resource("Material", mat_rc);
    RenderResource::register_resource("Camera", cam_rc);
}

RenderDevice *RenderEngine::get_device() { return device; }
void RenderEngine::register_mesh(Mesh *mesh) {
    this->mesh_instances[mesh] = {};
}

LinearAllocator *RenderEngine::get_mem_pool() { return &this->mem_pool; }

void RenderEngine::process() {
    std::vector<ModelEntity *> &entities =
        SeedEngine::get_instance()->get_world()->get_model_entities();
    for (ModelEntity *e : entities) {
        Ref<Mesh> mesh = e->get_mesh();
        if (mesh.is_null()) continue;
        std::vector<Mat4> &instances = mesh_instances[*mesh];
        Mat4 model = Mat4::translate_mat(e->get_position());
        Vec3 rotation = e->get_rotation();
        model *= Mat4::rotate_mat(rotation.z, Vec3{0, 0, 1});
        model *= Mat4::rotate_mat(rotation.y, Vec3{0, 1, 0});
        model *= Mat4::rotate_mat(rotation.x, Vec3{1, 0, 0});
        model *= Mat4::scale_mat(e->get_scale());
        instances.push_back(model.transpose());
    }

    RenderCommandDispatcher dp;
    /* color pass */
    for (const auto &[mesh, instances] : mesh_instances) {
        dp.begin();
        dp.use(&mesh->vertices_rc);
        dp.use(&mesh->vertices_desc_rc);
        dp.use(&mesh->indices_rc);
        dp.update(&mesh->instance_rc, 0, sizeof(Mat4) * instances.size(),
                  (void *)instances.data());
        dp.use(&mesh->instance_rc);
        dp.use(&mesh->instance_desc_rc);
        dp.render(this->default_shader, instances.size());
        dp.end();
    }

    for (auto &[mesh, instances] : mesh_instances) {
        instances.clear();
    }

    this->device->process();
    this->mem_pool.free_all();
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed