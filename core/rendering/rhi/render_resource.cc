#include "render_resource.h"
#include "core/io/path.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/macro.h"
#include <cstdlib>

namespace Seed {

namespace RHI {

VertexHandle alloc_vertex(u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data) {
    return System::gRenderEngine->get_device()->alloc_vertex(
        stride, element_cnt, frequence, data);
}
IndexHandle alloc_index(const std::vector<u8> &indices,
                        UpdateFrequence frequence) {
    return System::gRenderEngine->get_device()->alloc_indices(
        IndexType::UNSIGNED_BYTE, indices.size(), frequence, indices.data());
}
IndexHandle alloc_index(const std::vector<u16> &indices,
                        UpdateFrequence frequence) {
    return System::gRenderEngine->get_device()->alloc_indices(
        IndexType::UNSIGNED_SHORT, indices.size(), frequence, indices.data());
}
IndexHandle alloc_index(const std::vector<u32> &indices,
                        UpdateFrequence frequence) {
    return System::gRenderEngine->get_device()->alloc_indices(
        IndexType::UNSIGNED_INT, indices.size(), frequence, indices.data());
}

ConstantHandle alloc_constant(u32 size, UpdateFrequence frequence, void *data) {
    return System::gRenderEngine->get_device()->alloc_constant(size, data,
                                                               frequence);
}

TextureHandle alloc_texture(TextureType type, u32 w, u32 h, PixelFormat format,
                            MSAAType msaa_type, const void *data,
                            const SamplerProperty &property) {
    return System::gRenderEngine->get_device()->alloc_texture(
        type, w, h, format, msaa_type, property, data);
}

[[nodiscard]]
TextureHandle alloc_textures(TextureType type, u32 w, u32 h, PixelFormat format,
                             u32 count, const SamplerProperty &property) {
    return System::gRenderEngine->get_device()->alloc_textures(
        type, w, h, format, count, property);
}
[[nodiscard]]
TextureHandle alloc_cubemap(u32 w, u32 h, PixelFormat format,
                            const SamplerProperty &property) {
    return System::gRenderEngine->get_device()->alloc_cubemap(w, h, format,
                                                              property);
}

TextureHandle alloc_mappable_texture(TextureType type, u32 w, u32 h,
                                     PixelFormat format, const void *data,
                                     const SamplerProperty &property) {
    return System::gRenderEngine->get_device()->alloc_mappable_texture(
        type, w, h, format, property, data);
}

void query_texture_size(TextureHandle handle, u32 *w, u32 *h) {
    if (!w || !h) {
        return;
    }
    System::gRenderEngine->get_device()->query_texture_size(handle, w, h);
}

void blit_texture(TextureHandle dst, TextureHandle src, u32 dst_layer,
                  u32 src_layer, const Rect &dst_region,
                  const Rect &src_region) {
    if (dst == src && dst_layer == src_layer) {
        SEED_WARN("Blitting to same image with same layer, skipping.");
        return;
    }
    System::gRenderEngine->get_device()->blit_texture(
        dst, src, dst_layer, src_layer, dst_region, src_region);
}

SSBOHandle alloc_storage_buffer(u32 size, UpdateFrequence frequence,
                                void *data) {
    return System::gRenderEngine->get_device()->alloc_storage_buffer(size, data,
                                                                     frequence);
}

ShaderHandle alloc_shader(const Path &path, const KString &code,
                          ShaderLayout *layout,
                          const std::vector<ShaderDefine> &defines) {
    return System::gRenderEngine->compile_shader(path, code, layout, defines);
}

PipelineHandle alloc_pipeline(ShaderHandle shader,
                              const RenderRasterizerState &rst_state,
                              const RenderDepthStencilState &depth_state,
                              const RenderBlendState &blend_state) {
    return System::gRenderEngine->get_device()->alloc_pipeline(
        shader, rst_state, depth_state, blend_state);
}

RenderPassHandle alloc_renderpass() {
    return System::gRenderEngine->get_device()->alloc_render_pass();
}

UpdateBufferInfo alloc_heap(u32 size) {
    return UpdateBufferInfo{.data = malloc(size), .size = size};
}

UpdateBufferInfo alloc_texture_heap(PixelFormat format, u32 w, u32 h) {
    u32 size = get_pixel_format_size(format) * w * h;
    return UpdateBufferInfo{.data = malloc(size),
                            .image = {get_pixel_format_size(format), w, h}};
}

/* these commands will be execute at start of frame */
void update(VertexHandle handle, u32 offset, u32 size, const void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    UpdateBufferInfo heap = alloc_heap(size);
    memcpy(heap.data, data, size);
    System::gRenderEngine->get_device()->update(
        RenderResourceType::VERTEX, handle, offset, size, heap.data);
}

/* these commands will be execute at start of frame */
void update(IndexHandle handle, u32 offset, u32 size, const void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    UpdateBufferInfo heap = alloc_heap(size);
    memcpy(heap.data, data, size);
    System::gRenderEngine->get_device()->update(
        RenderResourceType::INDEX, handle, offset, size, heap.data);
}

/* these commands will be execute at start of frame */
void update(ConstantHandle handle, u32 offset, u32 size, const void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    UpdateBufferInfo heap = alloc_heap(size);
    memcpy(heap.data, data, size);
    System::gRenderEngine->get_device()->update(
        RenderResourceType::CONSTANT, handle, offset, size, heap.data);
}

/* these commands will be execute at start of frame */
void update(SSBOHandle handle, u32 offset, u32 size, const void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    UpdateBufferInfo heap = alloc_heap(size);
    memcpy(heap.data, data, size);
    System::gRenderEngine->get_device()->update(
        RenderResourceType::STORAGE_BUFFER, handle, offset, size, heap.data);
}

void update(TextureHandle handle, PixelFormat format, u32 layer, u32 offx,
            u32 offy, u32 w, u32 h, const void *data) {
    u64 size = w * h * get_pixel_format_size(format);
    if (size == 0 || data == nullptr) {
        return;
    }
    UpdateBufferInfo heap = alloc_heap(size);
    memcpy(heap.data, data, size);
    System::gRenderEngine->get_device()->update(handle, layer, offx, offy, w, h,
                                                heap.data);
}

void update_texture_sampler(TextureHandle handle, u32 layer,
                            const SamplerProperty &property) {
    System::gRenderEngine->get_device()->update_texture_sampler(handle, layer,
                                                                property);
}

/* these commands will be execute at start of frame */
void update_from_heap(VertexHandle handle, u32 offset, UpdateBufferInfo info) {
    System::gRenderEngine->get_device()->update(
        RenderResourceType::VERTEX, handle, offset, info.size, info.data);
}

/* these commands will be execute at start of frame */
void update_from_heap(IndexHandle handle, u32 offset, UpdateBufferInfo info) {
    System::gRenderEngine->get_device()->update(
        RenderResourceType::INDEX, handle, offset, info.size, info.data);
}

/* these commands will be execute at start of frame */
void update_from_heap(ConstantHandle handle, u32 offset,
                      UpdateBufferInfo info) {
    System::gRenderEngine->get_device()->update(
        RenderResourceType::CONSTANT, handle, offset, info.size, info.data);
}

/* these commands will be execute at start of frame */
void update_from_heap(SSBOHandle handle, u32 offset, UpdateBufferInfo info) {
    System::gRenderEngine->get_device()->update(
        RenderResourceType::STORAGE_BUFFER, handle, offset, info.size,
        info.data);
}

void update_from_heap(TextureHandle handle, u32 layer, u32 offx, u32 offy,
                      UpdateBufferInfo info) {
    if (info.data == nullptr) {
        SEED_WARN("Trying to upload heap with null data, skipping.");
        return;
    }
    System::gRenderEngine->get_device()->update(
        handle, layer, offx, offy, info.image.w, info.image.h, info.data);
}

void bind_depth_attachment(RenderPassHandle handle, TextureHandle texture,
                           u32 face) {
    System::gRenderEngine->get_device()->bind_depth_attachment(handle, texture,
                                                               face);
}
void bind_color_attachment(RenderPassHandle handle, u8 slot,
                           TextureHandle texture, u32 face) {
    System::gRenderEngine->get_device()->bind_color_attachment(handle, slot,
                                                               texture, face);
}
void *map_buffer(VertexHandle handle) {
    return System::gRenderEngine->get_device()->map_buffer(
        RenderResourceType::VERTEX, handle);
}
void *map_buffer(IndexHandle handle) {
    return System::gRenderEngine->get_device()->map_buffer(
        RenderResourceType::INDEX, handle);
}
void *map_buffer(ConstantHandle handle) {
    return System::gRenderEngine->get_device()->map_buffer(
        RenderResourceType::CONSTANT, handle);
}
void *map_buffer(SSBOHandle handle) {
    return System::gRenderEngine->get_device()->map_buffer(
        RenderResourceType::STORAGE_BUFFER, handle);
}
void *map_texture(TextureHandle handle) {
    return System::gRenderEngine->get_device()->map_texture(handle);
}

void dealloc(TextureHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::TEXTURE,
                                                 handle);
}
void dealloc(VertexHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::VERTEX,
                                                 handle);
}
void dealloc(IndexHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::INDEX,
                                                 handle);
}
void dealloc(ShaderHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::SHADER,
                                                 handle);
}
void dealloc(ConstantHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::CONSTANT,
                                                 handle);
}
void dealloc(PipelineHandle handle) {
    System::gRenderEngine->get_device()->dealloc(RenderResourceType::PIPELINE,
                                                 handle);
}
void dealloc(SSBOHandle handle) {
    System::gRenderEngine->get_device()->dealloc(
        RenderResourceType::STORAGE_BUFFER, handle);
}

void dealloc(RenderPassHandle handle) {
    System::gRenderEngine->get_device()->dealloc(
        RenderResourceType::RENDER_TARGET, handle);
}

}  // namespace RHI

};  // namespace Seed
