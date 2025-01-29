#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/rendering/camera.h"
#include "render_command.h"
#include "render_device.h"
#include "core/rendering/mesh.h"
#include "core/rendering/model.h"
#include "core/allocator/linear_allocator.h"
#include "core/rendering/render_pass/color_pass.h"
#include "core/rendering/render_pass/debug_pass.h"
#include <queue>
#include <vector>

namespace Seed {
class RenderEngine {
   private:

    inline static RenderEngine *instance = nullptr;
    RenderDevice *device;
    RenderResource *debug_shader;

    LinearAllocator mem_pool;
    ColorPass color_pass;
    DebugPass debug_pass;
   public:
    static RenderEngine *get_instance();
    void process();
    LinearAllocator *get_mem_pool();
    RenderDevice *get_device();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif