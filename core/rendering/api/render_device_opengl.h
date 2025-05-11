#ifndef _SEED_RENDER_DIVICE_OPENGL_H_
#define _SEED_RENDER_DIVICE_OPENGL_H_
#include "render_device.h"
#include "core/container/freelist.h"
#include "core/rendering/api/render_pipeline.h"
#include <map>

namespace Seed {
class RenderDeviceOpenGL : public RenderDevice {
   private:
    struct HardwareBufferGL{
        u32 handle = -1;
        u64 size;
    };
    u32 global_vao;
    u16 last_material = 0xffff;
    /* uniform buffer */
    std::map<std::string, RenderResource> constants;
    u32 constant_cnt = 0;
    std::map<u32, RenderResource> shaders;
    FreeList<HardwareBufferGL> buffers;
    void setup_rasterizer(RenderRasterizerState &state);
    void setup_depth_stencil(RenderDepthStencilState &state);
    void setup_blend(RenderBlendState&state);
    void handle_update(RenderCommand &cmd);
    void handle_state(RenderCommand &cmd);
    void handle_render(RenderCommand &cmd);
    void use_vertex_desc(VertexDescription *desc);
    void bind_buffer(RenderResource *rc);
   public:
    RenderDeviceOpenGL();
    ~RenderDeviceOpenGL() = default;

    void alloc_texture(RenderResource *rc, u32 w, u32 h, const void *data) override;
    void alloc_vertex(RenderResource *rc, u32 stride, u32 element_cnt,
        const void *data) override;
    void alloc_indices(RenderResource *rc, const std::vector<u32> &indices) override;
    void alloc_shader(RenderResource *rc, const std::string &vertex_code,
        const std::string &fragment_code,
        const std::string &geometry_code, const std::string &tess_ctrl_code, const std::string &tess_eval_code) override;
    void alloc_constant(RenderResource *rc, const std::string &name, u32 size,
                        void *data) override;
    void dealloc(RenderResource *r) override;

    void process() override;
};

}  // namespace Seed

#endif