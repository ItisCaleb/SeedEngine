#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "core/rendering/camera.h"
#include "render_command.h"
#include "render_device.h"
#include "core/rendering/mesh.h"
#include "core/allocator/linear_allocator.h"
#include <unordered_map>
#include <queue>
#include <vector>

namespace Seed {
class RenderEngine {
   private:
    inline static RenderEngine *instance = nullptr;
    RenderDevice *device;
    std::unordered_map<Mesh *,
                       std::unordered_map<Material *, std::vector<Mat4>>>
        mesh_instances;
    RenderResource *default_shader;
    RenderResource default_texture;
    LinearAllocator mem_pool;

   public:
    static RenderEngine *get_instance();
    void process();
    void register_mesh(Mesh *mesh);
    void remove_mesh(Mesh *mesh);
    LinearAllocator *get_mem_pool();
    RenderDevice *get_device();
    RenderEngine(GLFWwindow *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif