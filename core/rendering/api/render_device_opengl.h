#ifndef _SEED_RENDER_DIVICE_OPENGL_H_
#define _SEED_RENDER_DIVICE_OPENGL_H_
#include "render_device.h"
#include <map>



namespace Seed {
class RenderDeviceOpenGL : public RenderDevice {
   private:
    u32 element_cnt = 0;
    u32 vertex_cnt = 0;
    u32 global_vao;
    u32 current_program;
    std::vector<std::vector<VertexAttribute>> vertex_attrs;
    void handle_update(RenderCommand &cmd);
    void handle_use(RenderCommand &cmd);
    void handle_render(RenderCommand &cmd);
    void use_vertex_desc(u32 handle);
   public:
    RenderDeviceOpenGL();
    ~RenderDeviceOpenGL() = default;

    void alloc_texture(RenderResource *rc, u32 w, u32 h, void *data) override;
    void alloc_vertex(RenderResource *rc, u32 stride, u32 element_cnt,
                      void *data) override;
    void alloc_vertex_desc(RenderResource *rc, std::vector<VertexAttribute> &attrs) override;
    void alloc_indices(RenderResource *rc, std::vector<u32> &indices) override;
    void alloc_shader(RenderResource *rc, const char *vertex_code,
                      const char *fragment_code, const char *geometry_code) override;
    void alloc_constant(RenderResource *rc, u32 size, void *data) override;
    void dealloc(RenderResource *r) override;

    void process() override;
};

}  // namespace Seed

#endif