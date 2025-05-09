#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include "core/rendering/camera.h"
#include "render_command.h"
#include "render_device.h"
#include "core/rendering/mesh.h"
#include "core/resource/model.h"
#include "core/allocator/linear_allocator.h"
#include "core/rendering/renderer/renderer.h"
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
    std::vector<Renderer*> renderers;
    Ref<Texture> default_tex;
    Ref<Material> default_mat;
   public:
    static RenderEngine *get_instance();
    void process();
    LinearAllocator *get_mem_pool();
    RenderDevice *get_device();
    Camera *get_cam();
    Ref<Texture> get_default_texture(){
        return default_tex;
    }
    Ref<Material> get_default_material(){
        return default_mat;
    }
    template <class T>
    RenderEngine(T *window, int w, int h);
    ~RenderEngine();
};

}  // namespace Seed

#endif