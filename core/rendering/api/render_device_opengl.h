#ifndef _SEED_RENDER_DIVICE_OPENGL_H_
#define _SEED_RENDER_DIVICE_OPENGL_H_
#include "render_device.h"
#include "core/container/freelist.h"
#include "core/handle.h"
#include <glad/glad.h>
#include <map>
#include "core/rendering/api/render_common.h"

namespace Seed {

struct HardwareBufferGL {
        GLuint handle;
        u64 size;
};

struct HardwareIndexGL : public HardwareBufferGL {
        IndexType type;
};

struct HardwareConstantGL : public HardwareBufferGL {
        std::string name;
        GLuint buffer_base;
};

struct HardwareTextureGL {
        GLuint handle;
        u32 w, h;
        TextureType type;
};

struct HardwareShaderGL {
        GLuint handle;
        std::string vertex_src;
        std::string geo_src;
        std::string tess_ctrl_src;
        std::string tess_eval_src;
        std::string fragment_src;
};

struct HardwarePipelineGL {
        RenderResource shader;
        RenderRasterizerState rst_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_state;
};

struct HardwareRenderTargetGL {
        GLuint handle;
};

class RenderDeviceOpenGL : public RenderDevice {
    private:
        struct AllocCommand {
                Handle handle;
                RenderResourceType type;
                bool is_alloc;
        };
        GLuint global_vao;
        std::queue<AllocCommand> alloc_cmds;

        HandleOwner<HardwareBufferGL> buffers;
        HandleOwner<HardwareIndexGL> indices;
        HandleOwner<HardwareConstantGL> constants;
        HandleOwner<HardwareTextureGL> textures;
        HandleOwner<HardwareShaderGL> shaders;
        std::vector<Handle> shader_in_use;
        HandleOwner<HardwarePipelineGL> pipelines;
        HandleOwner<HardwareRenderTargetGL> render_targets;

        void find_samplers(const std::string &src,
                           std::vector<std::string> &result);
        GLuint convert_texture_type(TextureType type);

        /* state setup */
        void setup_rasterizer(const RenderRasterizerState &state);
        void setup_depth_stencil(const RenderDepthStencilState &state);
        void setup_blend(const RenderBlendState &state);

        /* allocating and drawing commands */
        void handle_alloc(AllocCommand &cmd);
        void handle_dealloc(AllocCommand &cmd);
        void handle_update(RenderCommand &cmd);
        void handle_state(RenderCommand &cmd);
        void handle_render(RenderCommand &cmd);

        /* binding operations */
        void use_vertex_desc(VertexDescription *desc);
        void bind_buffer(RenderResource &rc);
        void use_shader(RenderResource &rc);
        void use_texture(u32 unit, RenderResource &rc);

    public:
        RenderDeviceOpenGL();
        ~RenderDeviceOpenGL() = default;

        /* we defer the allocation to allow multithreading. */
        void alloc_texture(RenderResource *rc, TextureType type, u32 w,
                           u32 h) override;
        void alloc_vertex(RenderResource *rc, u32 stride,
                          u32 element_cnt) override;
        void alloc_indices(RenderResource *rc, IndexType type,
                           u32 element_cnt) override;
        void alloc_shader(RenderResource *rc, const std::string &vertex_code,
                          const std::string &fragment_code,
                          const std::string &geometry_code,
                          const std::string &tess_ctrl_code,
                          const std::string &tess_eval_code) override;
        void alloc_constant(RenderResource *rc, const std::string &name,
                            u32 size) override;
        void alloc_pipeline(RenderResource *rc, RenderResource shader,
                            const RenderRasterizerState &rst_state,
                            const RenderDepthStencilState &depth_state,
                            const RenderBlendState &blend_state) override;
        void alloc_render_target(RenderResource *rc) override;
        void dealloc(RenderResource *r) override;

        void process() override;
};

}  // namespace Seed

#endif