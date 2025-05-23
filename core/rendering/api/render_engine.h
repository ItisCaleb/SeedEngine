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

namespace Seed {
class RenderEngine {
    private:
        struct Layer {
                Viewport viewport;
                Renderer *renderer;
                Layer(Window *window, Renderer *rd)
                    : viewport(window), renderer(rd) {}
        };
        inline static RenderEngine *instance = nullptr;
        RenderBackend *device;
        RenderResource matrices_rc, cam_rc;
        Camera cam;
        LinearAllocator mem_pool;
        std::vector<Layer> layers;
        Window *current_window;

    public:
        static RenderEngine *get_instance();
        void init();
        void process();
        LinearAllocator *get_mem_pool();
        RenderBackend *get_device();
        Camera *get_cam();
        template <typename T, typename... Args>
        void register_renderer(u32 layer, const Args &...args);
        void set_layer_viewport(u32 layer, RectF rect);
        Viewport &get_layer_viewport(u32 layer);
        Window *get_current_window() { return current_window; }

        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif