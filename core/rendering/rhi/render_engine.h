#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include <vector>
#include "core/rendering/backend/render_backend.h"
#include "core/resource/model.h"
#include "core/rendering/renderer/renderer.h"
#include "core/window.h"
#include "core/rendering/mesh_storage.h"
#include "core/rendering/rhi/shader_proxy.h"

namespace Seed {
#define TRANSFORM_POOL_NAME "TransformDataPool"
#define TERRAIN_POOL_NAME "TerrainDataPool"
#define SKELETON_POOL_NAME "SkeletonInstancePool"

class RenderEngine {
    private:
        struct RendererLayer {
                u8 layer;
                bool enabled;
                Renderer *renderer;
        };
        inline static RenderEngine *instance = nullptr;
        RenderBackend *device;
        MeshStorage *mesh_storage;
        ShaderProxy *shader_proxy;
        std::vector<RendererLayer> renderers;
        std::unordered_map<std::string, InstanceDataPool *> instance_pools;
        Renderer *default_renderer;
        Renderer *imgui_renderer;

        Window *current_window;
        void bind_vulken(Window *window);
        void bind_vulkan_xr(Window *window);

    public:
        static RenderEngine *get_instance();
        void init();
        void process();
        RenderBackend *get_device();
        void register_renderer(u8 layer, Renderer *renderer);
        Window *get_current_window() { return current_window; }
        InstanceDataPool *get_instance_pool(const std::string &name);

        /* if not null, layout will be filled */
        ShaderHandle compile_shader(const Path &path, const std::string &shader,
                                    ShaderLayout *layout);
        Renderer *get_default_renderer() { return default_renderer; }
        Renderer *get_imgui_renderer() { return imgui_renderer; }
        void set_renderer_layer(Renderer *renderer, u8 layer);
        void set_renderer_enable(Renderer *renderer, bool enable);
        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif