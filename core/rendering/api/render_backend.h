#ifndef _SEED_RENDER_BACKEND_H_
#define _SEED_RENDER_BACKEND_H_
#include "render_command.h"
#include "render_resource.h"
#include "core/handle.h"

namespace Seed {

class RenderBackend {
    protected:
        std::deque<RenderCommand> cmd_queue;
        RenderResource current_pipeline;

    public:
        RenderBackend(/* args */) = default;
        ~RenderBackend() = default;
        virtual void alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                   u32 h) = 0;
        virtual void alloc_vertex(RenderResource *rc, u32 stride,
                                  u32 element_cnt) = 0;

        virtual void alloc_indices(RenderResource *rc, IndexType type,
                                   u32 element_cnt) = 0;
        virtual void alloc_shader(RenderResource *rc,
                                  const std::string &vertex_code,
                                  const std::string &fragment_code,
                                  const std::string &geometry_code,
                                  const std::string &tesselation_code,
                                  const std::string &tess_eval_code) = 0;
        virtual void alloc_constant(RenderResource *rc, const std::string &name,
                                    u32 size) = 0;
        virtual void alloc_pipeline(RenderResource *rc, RenderResource shader,
                                    const RenderRasterizerState &rst_state,
                                    const RenderDepthStencilState &depth_state,
                                    const RenderBlendState &blend_state) = 0;
        virtual void alloc_render_target(RenderResource *rc) = 0;
        virtual void dealloc(RenderResource *r) = 0;
        void push_cmd(RenderCommand &cmd) { this->cmd_queue.push_back(cmd); }
        virtual void process() = 0;
};

}  // namespace Seed

#endif