#include <seed/render_engine.h>
#include <seed/engine.h>
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

void RenderEngine::bind_shader(Ref<Shader> shader){
    this->shader = shader;
}
void RenderEngine::render(){
    if(shader.is_null()) return;
    shader->use();
    World *world = SeedEngine::get_instance()->get_world();
    if(!world) return;
    for(Entity *e : world->get_entities()){
        Ref<Mesh> m = e->get_mesh();
        if(m.is_null()) continue;
        glBindVertexArray(m->VAO);
        glDrawElements(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed