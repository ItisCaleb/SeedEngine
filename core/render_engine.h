#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "rendering/shader.h"
#include "rendering/camera.h"

namespace Seed {
class RenderEngine {
   private:
    inline static RenderEngine *instance = nullptr;
    Ref<Shader> shader;
    Camera *camera;

   public:
    static RenderEngine *get_instance();
    void bind_shader(Ref<Shader> shader);
    Camera *get_camera();
    void render();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif