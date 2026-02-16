#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

namespace RHI {

TextureHandle alloc_texture(TextureType type, u32 w, u32 h, PixelFormat format,
                            const void *data, const SamplerProperty &property) {
    return RenderEngine::get_instance()->get_device()->alloc_texture(
        type, w, h, format, property, data);
}
VertexHandle alloc_vertex(u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data) {
    return RenderEngine::get_instance()->get_device()->alloc_vertex(
        stride, element_cnt, frequence, data);
}
IndexHandle alloc_index(const std::vector<u8> &indices,
                        UpdateFrequence frequence) {
    return RenderEngine::get_instance()->get_device()->alloc_indices(
        IndexType::UNSIGNED_BYTE, indices.size(), frequence, indices.data());
}
IndexHandle alloc_index(const std::vector<u16> &indices,
                        UpdateFrequence frequence) {
    return RenderEngine::get_instance()->get_device()->alloc_indices(
        IndexType::UNSIGNED_SHORT, indices.size(), frequence, indices.data());
}
IndexHandle alloc_index(const std::vector<u32> &indices,
                        UpdateFrequence frequence) {
    return RenderEngine::get_instance()->get_device()->alloc_indices(
        IndexType::UNSIGNED_INT, indices.size(), frequence, indices.data());
}

ShaderHandle alloc_shader(const std::string &path, const std::string &code,
                          ShaderLayout *layout) {
    return RenderEngine::get_instance()->compile_shader(path, code, layout);
}
ConstantHandle alloc_constant(u32 size, UpdateFrequence frequence, void *data) {
    return RenderEngine::get_instance()->get_device()->alloc_constant(
        size, data, frequence);
}

SSBOHandle alloc_storage_buffer(u32 size, UpdateFrequence frequence,
                                void *data) {
    return RenderEngine::get_instance()->get_device()->alloc_storage_buffer(
        size, data, frequence);
}
PipelineHandle alloc_pipeline(ShaderHandle shader,
                              const RenderRasterizerState &rst_state,
                              const RenderDepthStencilState &depth_state,
                              const RenderBlendState &blend_state) {
    return RenderEngine::get_instance()->get_device()->alloc_pipeline(
        shader, rst_state, depth_state, blend_state);
}

RenderTargetHandle alloc_render_target(bool depth_only) {
    return RenderEngine::get_instance()->get_device()->alloc_render_target(
        depth_only);
}

void *alloc_heap(u32 size) { return malloc(size); }

/* these commands will be execute at start of frame */
void update(VertexHandle handle, u32 offset, u32 size, void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    void *heap = alloc_heap(size);
    memcpy(data, heap, size);
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update(IndexHandle handle, u32 offset, u32 size, void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    void *heap = alloc_heap(size);
    memcpy(data, heap, size);
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update(ConstantHandle handle, u32 offset, u32 size, void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    void *heap = alloc_heap(size);
    memcpy(data, heap, size);
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update(SSBOHandle handle, u32 offset, u32 size, void *data) {
    if (size == 0 || data == nullptr) {
        return;
    }
    void *heap = alloc_heap(size);
    memcpy(data, heap, size);
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

void update(TextureHandle handle, PixelFormat format, u32 layer, u32 offx,
            u32 offy, u32 w, u32 h, void *data) {
    u64 size = w * h * get_pixel_format_size(format);
    if (size == 0 || data == nullptr) {
        return;
    }
    void *heap = alloc_heap(size);
    memcpy(data, heap, size);
    RenderEngine::get_instance()->get_device()->update(handle, layer, offx,
                                                       offy, w, h, data);
}

/* these commands will be execute at start of frame */
void update_from_heap(VertexHandle handle, u32 offset, u32 size, void *data) {
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update_from_heap(IndexHandle handle, u32 offset, u32 size, void *data) {
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update_from_heap(ConstantHandle handle, u32 offset, u32 size, void *data) {
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}

/* these commands will be execute at start of frame */
void update_from_heap(SSBOHandle handle, u32 offset, u32 size, void *data) {
    RenderEngine::get_instance()->get_device()->update(handle, offset, size,
                                                       data);
}
void bind_depth_attachment(RenderTargetHandle handle, TextureHandle texture,
                           u32 face) {
    RenderEngine::get_instance()->get_device()->bind_depth_attachment(
        handle, texture, face);
}
void bind_color_attachment(RenderTargetHandle handle, u8 slot,
                           TextureHandle texture, u32 face) {
    RenderEngine::get_instance()->get_device()->bind_color_attachment(
        handle, slot, texture, face);
}
void dealloc(TextureHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(VertexHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(IndexHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(ShaderHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(ConstantHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(PipelineHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}
void dealloc(SSBOHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}

void dealloc(RenderTargetHandle handle) {
    RenderEngine::get_instance()->get_device()->dealloc(handle);
}

}  // namespace RHI

};  // namespace Seed
