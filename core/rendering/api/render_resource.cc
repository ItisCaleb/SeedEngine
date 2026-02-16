#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

void RenderResource::alloc_texture(TextureType type, u32 w, u32 h,
                                   PixelFormat format, const void *data,
                                   const SamplerProperty &property) {
    this->type = RenderResourceType::TEXTURE;
    RenderEngine::get_instance()->get_device()->alloc_texture(
        this, type, w, h, format, property, data);
}
void RenderResource::alloc_vertex(u32 stride, u32 vertex_cnt,
                                  UpdateFrequence frequence, const void *data) {
    this->type = RenderResourceType::VERTEX;
    RenderEngine::get_instance()->get_device()->alloc_vertex(
        this, stride, vertex_cnt, frequence, data);
}
void RenderResource::alloc_shader(const std::string &path,
                                  const std::string &code, ShaderLayout *layout) {
    this->type = RenderResourceType::SHADER;
    RenderEngine::get_instance()->compile_shader(this, path, code, layout);
}

void RenderResource::alloc_index(const std::vector<u8> &indices,
                                 UpdateFrequence frequence) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_BYTE, indices.size(), frequence,
        indices.data());
}
void RenderResource::alloc_index(const std::vector<u16> &indices,
                                 UpdateFrequence frequence) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_SHORT, indices.size(), frequence,
        indices.data());
}
void RenderResource::alloc_index(const std::vector<u32> &indices,
                                 UpdateFrequence frequence) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_INT, indices.size(), frequence,
        indices.data());
}

void RenderResource::alloc_constant(u32 size, void *data) {
    this->type = RenderResourceType::CONSTANT;
    RenderEngine::get_instance()->get_device()->alloc_constant(this, size,
                                                               data);
}
void RenderResource::alloc_pipeline(RenderResource shader,
                                    const RenderRasterizerState &rst_state,
                                    const RenderDepthStencilState &depth_state,
                                    const RenderBlendState &blend_state) {
    this->type = RenderResourceType::PIPELINE;
    RenderEngine::get_instance()->get_device()->alloc_pipeline(
        this, shader, rst_state, depth_state, blend_state);
}
void RenderResource::alloc_render_target(bool depth_only) {
    this->type = RenderResourceType::RENDER_TARGET;
    RenderEngine::get_instance()->get_device()->alloc_render_target(this,
                                                                    depth_only);
}

void RenderResource::alloc_buffer(u32 size, void *data) {
    this->type = RenderResourceType::BUFFER;
    RenderEngine::get_instance()->get_device()->alloc_buffer(this, size, data);
}

void RenderResource::dealloc() {
    if (this->type == RenderResourceType::UNINITIALIZE) {
        return;
    }
    RenderEngine::get_instance()->get_device()->dealloc(this);
    this->handle = 0;
    this->type = RenderResourceType::UNINITIALIZE;
}

bool RenderResource::inited() {
    return this->type != RenderResourceType::UNINITIALIZE;
}

}  // namespace Seed