#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/rendering/renderer/renderer.h"
#include "core/rendering/instance_batch.h"
#include "core/macro.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {

class InstanceBatchPool;
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
        std::unordered_map<u64, InstanceBatchPool *> instance_pools;
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

        template <typename T>
        void register_instance_batch(u32 element_size, u32 pool_size) {
            static_assert(std::is_base_of<InstanceBatch, T>::value,
                          "T must be a derived class of InstanceBatch.");
            constexpr u64 id = type_id<T>();
            auto iter = instance_pools.find(id);
            if (iter != instance_pools.end()) {
                SEED_WARN("Already registered instance type {}, skipping.",
                          type_name<T>);
                return;
            }
            instance_pools[type_id<T>()] =
                new InstanceBatchPool(element_size, pool_size);
        }

        InstanceBatchPool *get_instance_pool(u64 tid);
        InstanceBatchPool *get_instance_pool(Ref<InstanceBatch> batch);

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
