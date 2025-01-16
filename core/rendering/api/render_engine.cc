#include "render_engine.h"
#include "core/engine.h"
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
    glViewport(0, 0, w, h);

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
    glClear(GL_COLOR_BUFFER_BIT);
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