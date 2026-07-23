#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/rendering/renderer/renderer.h"

namespace Seed {
#define TRANSFORM_POOL_NAME "TransformDataPool"
#define TERRAIN_POOL_NAME "TerrainDataPool"
#define SKELETON_POOL_NAME "SkeletonInstancePool"

class InstanceBatchPool;
class MeshStorage;
class RenderBackend;
class ShaderLayout;
class ShaderProxy;
class Window;
struct ShaderDefine;

class RenderEngine {
    private:
        FrameGlobal g_frame;
        struct RendererLayer {
                u8 layer;
                bool enabled;
                Renderer *renderer;
        };
        RenderBackend *device;
        MeshStorage *mesh_storage;
        ShaderProxy *shader_proxy;
        std::vector<RendererLayer> renderers;
        std::unordered_map<std::string, InstanceBatchPool *> instance_pools;
        Renderer *default_renderer;
        Renderer *imgui_renderer;
        Renderer *rml_renderer;

        Window *current_window;
        void bind_vulken(Window *window);
        void bind_vulkan_xr(Window *window);

    public:
        void init();
        void process();
        RenderBackend *get_device();
        void register_renderer(u8 layer, Renderer *renderer);
        Window *get_current_window() { return current_window; }
        InstanceBatchPool *get_instance_pool(const std::string &name);

        /* if not null, layout will be filled */
        ShaderHandle compile_shader(const Path &path, const KString &shader,
                                    ShaderLayout *layout,
                                    const std::vector<ShaderDefine> &defines);
        Renderer *get_default_renderer() { return default_renderer; }
        Renderer *get_imgui_renderer() { return imgui_renderer; }
        Renderer *get_rml_renderer() { return rml_renderer; }
        MeshStorage *get_mesh_storage() { return mesh_storage; }
        void set_renderer_layer(Renderer *renderer, u8 layer);
        void set_renderer_enable(Renderer *renderer, bool enable);
        FrameGlobal &get_frame_global() { return g_frame; }
        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif
