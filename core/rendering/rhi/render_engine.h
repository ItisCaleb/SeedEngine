#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include <queue>
#include <vector>
#include "core/rendering/camera.h"
#include "core/rendering/rhi/render_command.h"
#include "core/rendering/backend/render_backend.h"
#include "core/rendering/mesh.h"
#include "core/resource/model.h"
#include "core/rendering/renderer/renderer.h"
#include "core/window.h"
#include "core/rendering/viewport.h"
#include "core/rendering/mesh_storage.h"
#include "core/rendering/rhi/shader_proxy.h"

namespace Seed {
class RenderEngine {
    private:
        inline static RenderEngine *instance = nullptr;
        RenderBackend *device;
        MeshStorage *mesh_storage;
        ShaderProxy *shader_proxy;
        std::vector<Renderer *> renderers;
        std::unordered_map<std::string, InstanceDataPool *> instance_pools;

        Window *current_window;
        void bind_opengl(Window *window);
        void bind_vulken(Window *window);

    public:
        static RenderEngine *get_instance();
        void init();
        void process();
        RenderBackend *get_device();
        template <typename T, typename... Args>
        void register_renderer(u32 layer, const Args &...args);
        Window *get_current_window() { return current_window; }
        InstanceDataPool *get_instance_pool(const std::string &name);

        /* if not null, layout will be filled */
        ShaderHandle compile_shader(const std::string &path,
                                    const std::string &shader,
                                    ShaderLayout *layout);

        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif