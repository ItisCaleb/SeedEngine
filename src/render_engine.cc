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
    this->camera = new Camera(Vec3{0, 0, 2}, Vec3{0,1,0}, Vec3{0, 0, -1});
    this->camera->set_perspective(45, 1.33, 0.1, 1000.0);
}

void RenderEngine::bind_shader(Ref<Shader> shader){
    this->shader = shader;
}
void RenderEngine::render(){
    if(shader.is_null()) return;
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shader->use();
    World *world = SeedEngine::get_instance()->get_world();
    if(!world) return;
    for(Entity *e : world->get_entities()){
        Mat4 projection = camera->perspective();
        Mat4 view = camera->look_at();
        Mat4 model = Mat4::translate_mat(e->get_position());
        Vec3 rotation = e->get_rotation();
        model *= Mat4::rotate_mat(rotation.z, Vec3{0, 0, 1});
        model *= Mat4::rotate_mat(rotation.y, Vec3{0, 1, 0});
        model *= Mat4::rotate_mat(rotation.x, Vec3{1, 0, 0});
        model *= Mat4::scale_mat(e->get_scale());
        // fmt::println("{} {} {} {}",model[0].x, model[0].y, model[0].z, model[0].w);
        // fmt::println("{} {} {} {}",model[1].x, model[1].y, model[1].z, model[1].w);
        // fmt::println("{} {} {} {}",model[2].x, model[2].y, model[2].z, model[2].w);
        // fmt::println("{} {} {} {}",model[3].x, model[3].y, model[3].z, model[3].w);
        // fmt::println("-----");
        shader->uniform_apply<Mat4*>("projection", &projection);
        shader->uniform_apply<Mat4*>("view", &view);
        shader->uniform_apply<Mat4*>("model", &model);
        Ref<Mesh> m = e->get_mesh();
        if(m.is_null()) continue;
        glBindVertexArray(m->VAO);
        glDrawElements(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    
}

Camera *RenderEngine::get_camera(){
    return camera;
}

RenderEngine::~RenderEngine() { instance = nullptr; }
}  // namespace Seed