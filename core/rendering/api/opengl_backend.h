#ifndef _SEED_OPENGL_BACKEND_H_
#define _SEED_OPENGL_BACKEND_H_
#include "render_backend.h"
#include "core/container/freelist.h"
#include "core/handle.h"
#include <glad/glad.h>
#include <map>
#include "core/rendering/render_common.h"

namespace Seed {

struct HardwareBufferGL {
        GLuint handle = GL_INVALID_INDEX;
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
        GLuint handle = GL_INVALID_INDEX;
        u32 w, h;
        TextureType type;
        PixelFormat format;
};

struct HardwareShaderGL {
        GLuint handle = GL_INVALID_INDEX;
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
        GLuint fbo;
        bool depth_only;
};


class RenderBackendGL : public RenderBackend {
    private:
        struct AllocCommand {
                RenderResource rc;
                bool is_alloc;
        };
        GLuint global_vao;
        GLuint last_fbo = 0;
        std::queue<AllocCommand> alloc_cmds;

        HandleOwner<HardwareBufferGL> vertices;
        HandleOwner<HardwareIndexGL> indices;
        HandleOwner<HardwareConstantGL> constants;
        HandleOwner<HardwareTextureGL> textures;
        HandleOwner<HardwareShaderGL> shaders;
        HandleOwner<HardwarePipelineGL> pipelines;
        HandleOwner<HardwareRenderTargetGL> render_targets;
        HandleOwner<HardwareBufferGL> ssbos;

        void find_samplers(const std::string &src,
                           std::vector<std::string> &result);
        GLuint convert_texture_type(TextureType type);
        GLuint convert_pixel_format(PixelFormat format);

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
        void use_vertex_desc(VertexLayout *desc);
        void bind_buffer(RenderResource &rc);
        void use_shader(RenderResource &rc);
        void use_texture(u32 unit, RenderResource &rc);

    public:
        RenderBackendGL();
        ~RenderBackendGL() = default;

        /* we defer the allocation to allow multithreading. */
        void alloc_texture(RenderResource *rc, TextureType type, u32 w, u32 h,
                           PixelFormat format) override;
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
        void alloc_render_target(RenderResource *rc, bool depth_only) override;
        void alloc_buffer(RenderResource *rc, u32 size) override;
        void dealloc(RenderResource *r) override;

        void process() override;
};

}  // namespace Seed

#endif