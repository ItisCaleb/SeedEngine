#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <seed/rendering/shader.h>

namespace Seed {
class RenderEngine {
   private:
    inline static RenderEngine *instance = nullptr;
    Ref<Shader> shader;
   public:
    static RenderEngine *get_instance();
    void bind_shader(Ref<Shader> shader);
    void render();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif