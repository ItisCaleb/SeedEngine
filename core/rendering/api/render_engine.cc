#include "render_engine.h"
#include "core/engine.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>
#include "core/rendering/light.h"
#include "core/rendering/material.h"

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
    glUseProgram(shader_rc.handle);

    struct Lights {
        PosLight p_light;
        DirLight d_light;
    } lights;
    lights.p_light = {Vec3{0, 3, 3}, Vec3{0.3, 0.3, 0.3}, Vec3{0.8, 0.4, 0.6},
                      Vec3{1, 1, 1}};

    lights.d_light = {Vec3{0.5, 0.5, 0.5}, Vec3{0.2, 0.2, 0.2}, Vec3{0.7, 0.7, 0.7},
                      Vec3{1, 1, 1}};

    RenderResource matrices_rc, lights_rc, mat_rc, cam_rc;

    matrices_rc.alloc_constant(sizeof(Mat4) * 3, NULL);
    lights_rc.alloc_constant(sizeof(Lights), &lights);
    mat_rc.alloc_constant(sizeof(Material), NULL);
    cam_rc.alloc_constant(sizeof(Vec3), NULL);

    RenderResource::register_resource("Matrices", matrices_rc);
    RenderResource::register_resource("Lights", lights_rc);
    RenderResource::register_resource("Material", mat_rc);
    RenderResource::register_resource("Camera", cam_rc);
}

void RenderEngine::handle_update(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;
    switch (rc->type) {
        case RenderResourceType::VERTEX:
            u32 vbo;
            glBindVertexArray(rc->handle);
            glBufferSubData(GL_ARRAY_BUFFER, cmd.offset, cmd.size, cmd.data);
            glBindVertexArray(0);
            free(cmd.data);
            break;
        case RenderResourceType::TEXTURE:
            break;
        case RenderResourceType::CONSTANT:
            glBindBuffer(GL_UNIFORM_BUFFER, rc->handle);
            glBufferSubData(GL_UNIFORM_BUFFER, cmd.offset, cmd.size, cmd.data);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        default:
            break;
    }
}
void RenderEngine::handle_use(RenderCommand &cmd) {
    RenderResource *rc = cmd.resource;

    switch (rc->type) {
        case RenderResourceType::VERTEX:
            glBindVertexArray(rc->handle);
            element_cnt = rc->element_cnt;
            break;
        case RenderResourceType::TEXTURE:
            break;
        case RenderResourceType::SHADER:
            glUseProgram(rc->handle);
            break;
        default:
            break;
    }
}

void RenderEngine::process() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        cmd_queue.pop();

        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                handle_update(cmd);
                break;
            case RenderCommandType::USE:
                handle_use(cmd);
                break;
            case RenderCommandType::RENDER:
                glDrawArrays(GL_TRIANGLES, 0, element_cnt);
                break;
            default:
                break;
        }
    }

    glBindVertexArray(0);
}

void RenderEngine::push_cmd(RenderCommand &cmd) { cmd_queue.push(cmd); }

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed