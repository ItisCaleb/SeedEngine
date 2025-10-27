#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include "core/rendering/camera.h"
#include "render_command.h"
#include "render_backend.h"
#include "core/rendering/mesh.h"
#include "core/resource/model.h"
#include "core/allocator/linear_allocator.h"
#include "core/rendering/renderer/renderer.h"
#include "core/window.h"
#include <queue>
#include <vector>
#include "core/rendering/viewport.h"
#include "core/rendering/render_target.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {
class RenderEngine {
    private:
        struct Layer {
                Ref<RenderTarget> rt;
                Renderer *renderer;
                Layer(Ref<RenderTarget> rt, Renderer *rd)
                    : rt(rt), renderer(rd) {}
        };
        inline static RenderEngine *instance = nullptr;
        RenderBackend *device;
        MeshStorage *mesh_storage;
        RenderResource matrices_rc, cam_rc;
        Camera cam;
        std::vector<Layer> layers;
        std::unordered_map<std::string, Ref<RenderTarget>> render_targets;
        std::unordered_map<std::string, InstanceDataPool *> instance_pools;

        Window *current_window;

    public:
        static RenderEngine *get_instance();
        void init();
        void process();
        RenderBackend *get_device();
        Camera *get_cam();
        template <typename T, typename... Args>
        void register_renderer(u32 layer, Ref<RenderTarget> rt, const Args &...args);
        Window *get_current_window() { return current_window; }
        Ref<RenderTarget> get_render_target(const std::string &name);
        InstanceDataPool *get_instance_pool(const std::string &name);

        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif