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
#include "core/rendering/renderer/model_renderer.h"
#include "core/rendering/renderer/terrain_renderer.h"
#include <queue>
#include <vector>

namespace Seed {
class RenderEngine {
   private:

    inline static RenderEngine *instance = nullptr;
    RenderDevice *device;
    RenderResource matrices_rc, cam_rc;
    Camera cam;
    LinearAllocator mem_pool;
    ModelRenderer model_renderer;
    TerrainRenderer terrain_renderer;
   public:
    static RenderEngine *get_instance();
    void process();
    LinearAllocator *get_mem_pool();
    RenderDevice *get_device();
    Camera *get_cam();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif