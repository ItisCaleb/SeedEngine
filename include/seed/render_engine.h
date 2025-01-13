#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Seed {
class RenderEngine {
   private:
    inline static RenderEngine *instance = nullptr;

   public:
    static RenderEngine *get_instance();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif