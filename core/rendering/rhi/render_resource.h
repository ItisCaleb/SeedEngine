#ifndef _SEED_RENDERING_RESOURCE_H_
#define _SEED_RENDERING_RESOURCE_H_
#include "core/io/path.h"
#include "core/types.h"
#include <vector>
#include <string>
#include "core/rendering/render_common.h"
#include "core/handle.h"
#include "core/rendering/shader_layout.h"

namespace Seed {

enum class RenderResourceType : u8 {
    TEXTURE,
    VERTEX,
    INDEX,
    CONSTANT,
    STORAGE_BUFFER,
    SHADER,
    PIPELINE,
    RENDER_TARGET
};

struct TextureTag;
struct VertexTag;
struct IndexTag;
struct ConstantTag;
struct SSBOTag;
struct ShaderTag;
struct PipelineTag;
struct RenderTargetTag;

typedef TypedHandle<TextureTag> TextureHandle;
typedef TypedHandle<VertexTag> VertexHandle;
typedef TypedHandle<IndexTag> IndexHandle;
typedef TypedHandle<ConstantTag> ConstantHandle;
typedef TypedHandle<SSBOTag> SSBOHandle;
typedef TypedHandle<ShaderTag> ShaderHandle;
typedef TypedHandle<PipelineTag> PipelineHandle;
typedef TypedHandle<RenderTargetTag> RenderPassHandle;

namespace RHI {

[[nodiscard]]
VertexHandle alloc_vertex(u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data);

[[nodiscard]]
IndexHandle alloc_index(const std::vector<u8> &indices,
                        UpdateFrequence frequence);

[[nodiscard]]
IndexHandle alloc_index(const std::vector<u16> &indices,
                        UpdateFrequence frequence);

[[nodiscard]]
IndexHandle alloc_index(const std::vector<u32> &indices,
                        UpdateFrequence frequence);

[[nodiscard]]
ConstantHandle alloc_constant(u32 size, UpdateFrequence frequence,
                              void *data = nullptr);

[[nodiscard]]
SSBOHandle alloc_storage_buffer(u32 size, UpdateFrequence frequence,
                                void *data = nullptr);

[[nodiscard]]
TextureHandle alloc_texture(TextureType type, u32 w, u32 h, PixelFormat format,
                            MSAAType msaa_type, const void *data,
                            const SamplerProperty &property);

[[nodiscard]]
TextureHandle alloc_textures(TextureType type, u32 w, u32 h, PixelFormat format,
                             u32 count, const SamplerProperty &property);
[[nodiscard]]
TextureHandle alloc_cubemap(u32 w, u32 h, PixelFormat format,
                            const SamplerProperty &property);

[[nodiscard]]
TextureHandle alloc_mappable_texture(TextureType type, u32 w, u32 h,
                                     PixelFormat format, const void *data,
                                     const SamplerProperty &property);
void query_texture_size(TextureHandle handle, u32 *w, u32 *h);

[[nodiscard]]
ShaderHandle alloc_shader(const Path &path, const std::string &code,
                          ShaderLayout *layout);

[[nodiscard]]
PipelineHandle alloc_pipeline(ShaderHandle shader,
                              const RenderRasterizerState &rst_state,
                              const RenderDepthStencilState &depth_state,
                              const RenderBlendState &blend_state);

[[nodiscard]]
RenderPassHandle alloc_renderpass();

struct UpdateBufferInfo {
        void *data;
        union {
                u64 size;
                struct {
                        u32 pixel_size : 8;
                        u32 w : 24;
                        u32 h : 24;
                } image;
        };
};

UpdateBufferInfo alloc_heap(u32 size);
UpdateBufferInfo alloc_texture_heap(PixelFormat format, u32 w, u32 h);

/* these commands will be execute at start of frame */
void update(VertexHandle handle, u32 offset, u32 size, const void *data);

/* these commands will be execute at start of frame */
void update(IndexHandle handle, u32 offset, u32 size, const void *data);

/* these commands will be execute at start of frame */
void update(ConstantHandle handle, u32 offset, u32 size, const void *data);

/* these commands will be execute at start of frame */
void update(SSBOHandle handle, u32 offset, u32 size, const void *data);

/* these commands will be execute at start of frame */
void update(TextureHandle handle, PixelFormat format, u32 layer, u32 offx,
            u32 offy, u32 w, u32 h, const void *data);

void update_texture_sampler(TextureHandle handle, u32 layer,
                            const SamplerProperty &property);

/* these commands will be execute at start of frame */
void update_from_heap(VertexHandle handle, u32 offset, UpdateBufferInfo info);

/* these commands will be execute at start of frame */
void update_from_heap(IndexHandle handle, u32 offset, UpdateBufferInfo info);

/* these commands will be execute at start of frame */
void update_from_heap(ConstantHandle handle, u32 offset, UpdateBufferInfo info);

/* these commands will be execute at start of frame */
void update_from_heap(SSBOHandle handle, u32 offset, UpdateBufferInfo info);

/* these commands will be execute at start of frame */
void update_from_heap(TextureHandle handle, u32 layer, u32 offx, u32 offy,
                      UpdateBufferInfo info);

void bind_depth_attachment(RenderPassHandle handle, TextureHandle texture,
                           u32 face);
void bind_color_attachment(RenderPassHandle handle, u8 slot,
                           TextureHandle texture, u32 face);

void *map_buffer(VertexHandle handle);
void *map_buffer(IndexHandle handle);
void *map_buffer(ConstantHandle handle);
void *map_buffer(SSBOHandle handle);
void *map_texture(TextureHandle handle);

void dealloc(TextureHandle handle);
void dealloc(VertexHandle handle);
void dealloc(IndexHandle handle);
void dealloc(ShaderHandle handle);
void dealloc(ConstantHandle handle);
void dealloc(PipelineHandle handle);
void dealloc(SSBOHandle handle);
void dealloc(RenderPassHandle handle);

};  // namespace RHI
}  // namespace Seed

#endif