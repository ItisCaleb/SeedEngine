#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/rendering/shader.h"
#include "core/rendering/camera.h"
#include "render_command.h"
#include <queue>

namespace Seed {
class RenderEngine {
   private:
    inline static RenderEngine *instance = nullptr;
    std::queue<RenderCommand> cmd_queue;
    u32 element_cnt = 0;
    void handle_update(RenderCommand &cmd);
    void handle_use(RenderCommand &cmd);

   public:
    static RenderEngine *get_instance();
    void push_cmd(RenderCommand &cmd);
    void process();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif