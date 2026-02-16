#ifndef _SEED_RENDERING_RESOURCE_H_
#define _SEED_RENDERING_RESOURCE_H_
#include "core/types.h"
#include <vector>
#include <string>
#include <map>
#include "core/rendering/render_common.h"
#include "core/handle.h"
#include "core/rendering/shader_layout.h"

namespace Seed {
enum class RenderResourceType : u8 {
    TEXTURE,
    VERTEX,
    INDEX,
    CONSTANT,
    BUFFER,
    SHADER,
    PIPELINE,
    RENDER_TARGET,
    UNINITIALIZE
};

struct RenderResource {
        Handle handle = NULL_HANDLE;
        RenderResourceType type = RenderResourceType::UNINITIALIZE;

        void alloc_texture(TextureType type, u32 w, u32 h, PixelFormat format,
                           const void *data, const SamplerProperty &property);
        void alloc_vertex(u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data);
        void alloc_index(const std::vector<u8> &indices,
                         UpdateFrequence frequence);
        void alloc_index(const std::vector<u16> &indices,
                         UpdateFrequence frequence);
        void alloc_index(const std::vector<u32> &indices,
                         UpdateFrequence frequence);

        void alloc_shader(const std::string &path, const std::string &code, ShaderLayout *layout);
        void alloc_constant(u32 size, void *data);
        void alloc_pipeline(RenderResource shader,
                            const RenderRasterizerState &rst_state,
                            const RenderDepthStencilState &depth_state,
                            const RenderBlendState &blend_state);

        void alloc_render_target(bool depth_only);
        void alloc_buffer(u32 size, void *data);
        void dealloc();
        bool inited();

        Handle get_handle() {
            /* retrieve the right 24 bits */
            return this->handle & 0xffffff;
        }
        RenderResourceType get_type() {
            return static_cast<RenderResourceType>(this->handle >> 24);
        }

        RenderResource() = default;
        ~RenderResource() = default;
};

}  // namespace Seed

#endif