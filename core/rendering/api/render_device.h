#ifndef _SEED_RENDER_DIVICE_H_
#define _SEED_RENDER_DIVICE_H_
#include "render_command.h"
#include "render_resource.h"

namespace Seed {
class RenderDevice {
   protected:
    std::deque<RenderCommand> cmd_queue;

   public:
    RenderDevice(/* args */) = default;
    ~RenderDevice() = default;
    virtual void alloc_texture(RenderResource *rc, u32 w, u32 h,
                               void *data) = 0;
    virtual void alloc_vertex(RenderResource *rc, u32 stride, u32 element_cnt,
                              void *data) = 0;
    virtual void alloc_vertex_desc(RenderResource *rc,
                                   std::vector<VertexAttribute> &attrs) = 0;

    virtual void alloc_indices(RenderResource *rc,
                               std::vector<u32> &indices) = 0;
    virtual void alloc_shader(RenderResource *rc, const char *vertex_code,
                              const char *fragment_code) = 0;
    virtual void alloc_constant(RenderResource *rc, u32 size, void *data) = 0;
    virtual void dealloc(RenderResource *r) = 0;
    void push_cmd(RenderCommand &cmd) { this->cmd_queue.push_back(cmd); }
    virtual void process() = 0;
};

}  // namespace Seed

#endif