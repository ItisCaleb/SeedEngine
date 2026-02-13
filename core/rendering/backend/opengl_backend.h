#ifndef _SEED_OPENGL_BACKEND_H_
#define _SEED_OPENGL_BACKEND_H_
#include "render_backend.h"
#include "core/container/freelist.h"
#include "core/handle.h"
#include <map>
#include "core/rendering/render_common.h"

namespace Seed {

#define GLuint u32
#define GL_INVALID_INDEX (-1)

struct HardwareBufferGL {
        GLuint handle = GL_INVALID_INDEX;
        u64 size;
};

struct HardwareIndexGL : public HardwareBufferGL {
        IndexType type;
};

struct HardwareTextureGL {
        GLuint handle = GL_INVALID_INDEX;
        u32 w, h;
        TextureType type;
        PixelFormat format;
        SamplerProperty property;
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
                void *alloc_data = nullptr;
        };
        GLuint global_vao;
        GLuint last_fbo = 0;
        RenderResource push_constant;

        std::mutex alloc_lock;
        std::queue<AllocCommand> alloc_cmds;

        HandleOwner<HardwareBufferGL> vertices;
        HandleOwner<HardwareIndexGL> indices;
        HandleOwner<HardwareBufferGL> ubos;
        HandleOwner<HardwareTextureGL> textures;
        HandleOwner<HardwareShaderGL> shaders;
        HandleOwner<HardwarePipelineGL> pipelines;
        HandleOwner<HardwareRenderTargetGL> render_targets;
        HandleOwner<HardwareBufferGL> ssbos;

        /* state setup */
        void setup_rasterizer(const RenderRasterizerState &state);
        void setup_depth_stencil(const RenderDepthStencilState &state);
        void setup_blend(const RenderBlendState &state);

        /* allocating and drawing commands */
        /* for multithreading purpose */
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
        inline RenderBackendType get_type() override {
            return RenderBackendType::OPENGL;
        }
        /* we defer the allocation to allow multithreading. */
        void alloc_texture(RenderResource *rc, TextureType type, u32 w, u32 h,
                           PixelFormat format, const SamplerProperty &property,
                           const void *data) override;
        void alloc_vertex(RenderResource *rc, u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data) override;
        void alloc_indices(RenderResource *rc, IndexType type, u32 element_cnt,
                           UpdateFrequence frequence,
                           const void *data) override;
        void alloc_shader(RenderResource *rc, const std::string &vertex_code,
                          const std::string &fragment_code,
                          const std::string &geometry_code,
                          const std::string &tess_ctrl_code,
                          const std::string &tess_eval_code) override;
        void setup_shader_layout(RenderResource *rc,
                                 const ShaderLayout &layout) override {};

        void alloc_constant(RenderResource *rc, u32 size,
                            const void *data) override;
        void alloc_pipeline(RenderResource *rc, RenderResource shader,
                            const RenderRasterizerState &rst_state,
                            const RenderDepthStencilState &depth_state,
                            const RenderBlendState &blend_state) override;
        void alloc_render_target(RenderResource *rc, bool depth_only) override;
        void alloc_buffer(RenderResource *rc, u32 size,
                          const void *data) override;
        void dealloc(RenderResource *r) override;
        void process_commands(std::deque<RenderCommand> &cmd_queue) override;
        void swap_buffer() override;
};

}  // namespace Seed

#endif