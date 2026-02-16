#ifndef _SEED_RENDER_ENGINE_H_
#define _SEED_RENDER_ENGINE_H_

#include <queue>
#include <vector>
#include "core/rendering/camera.h"
#include "core/rendering/api/render_command.h"
#include "core/rendering/backend/render_backend.h"
#include "core/rendering/mesh.h"
#include "core/resource/model.h"
#include "core/rendering/renderer/renderer.h"
#include "core/window.h"
#include "core/rendering/viewport.h"
#include "core/rendering/render_target.h"
#include "core/rendering/mesh_storage.h"
#include "core/rendering/api/shader_proxy.h"

namespace Seed {
class RenderEngine {
    friend DefaultRenderer;
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
        ShaderProxy *shader_proxy;
        RenderResource matrices_rc, cam_rc;
        RenderResource visible_ssbo;
        Camera cam;
        std::vector<Layer> layers;
        std::unordered_map<std::string, Ref<RenderTarget>> render_targets;
        std::unordered_map<std::string, InstanceDataPool *> instance_pools;

        Window *current_window;
        void bind_opengl(Window *window);
        void bind_vulken(Window *window);


        Mat4 get_window_projection();
    public:
        static RenderEngine *get_instance();
        void init();
        void process();
        RenderBackend *get_device();
        Camera *get_cam();
        template <typename T, typename... Args>
        void register_renderer(u32 layer, Ref<RenderTarget> rt,
                               const Args &...args);
        Window *get_current_window() { return current_window; }
        Ref<RenderTarget> get_render_target(const std::string &name);
        InstanceDataPool *get_instance_pool(const std::string &name);

        /* if not null, layout will be filled */
        void compile_shader(RenderResource *rc, const std::string &path,
                            const std::string &shader, ShaderLayout *layout);

        RenderEngine(Window *window);
        ~RenderEngine();
};

}  // namespace Seed

#endif