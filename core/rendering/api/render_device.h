#ifndef _SEED_RENDER_DIVICE_H_
#define _SEED_RENDER_DIVICE_H_
#include "render_command.h"
#include "render_resource.h"

namespace Seed {
class RenderDevice {
    protected:
        std::deque<RenderCommand> cmd_queue;
        Ref<RenderPipeline> pipeline;

    public:
        RenderDevice(/* args */) = default;
        ~RenderDevice() = default;
        virtual void alloc_texture(RenderResource *rc, u32 w, u32 h,
                                   const void *data) = 0;
        virtual void alloc_vertex(RenderResource *rc, u32 stride,
                                  u32 element_cnt, const void *data) = 0;

        virtual void alloc_indices(RenderResource *rc, IndexType type,
                                   u32 element_cnt, void *data) = 0;
        virtual void alloc_shader(RenderResource *rc,
                                  const std::string &vertex_code,
                                  const std::string &fragment_code,
                                  const std::string &geometry_code,
                                  const std::string &tesselation_code,
                                  const std::string &tess_eval_code) = 0;
        virtual void alloc_constant(RenderResource *rc, const std::string &name,
                                    u32 size, void *data) = 0;
        virtual void dealloc(RenderResource *r) = 0;
        void push_cmd(RenderCommand &cmd) { this->cmd_queue.push_back(cmd); }
        virtual void process() = 0;
};

}  // namespace Seed

#endif