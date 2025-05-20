#include "render_resource.h"
#include <glad/glad.h>
#include <fmt/core.h>
#include "render_engine.h"
#include <spdlog/spdlog.h>

namespace Seed {

void RenderResource::alloc_texture(TextureType type, u32 w, u32 h,
                                   const void *data) {
    this->type = RenderResourceType::TEXTURE;
    RenderEngine::get_instance()->get_device()->alloc_texture(this, type, w, h);
    if (data) {
        if (type == TextureType::TEXTURE_CUBEMAP) {
            SPDLOG_WARN("Cubemap texture will not updload data at allocation.");
            return;
        }
        RenderCommandDispatcher dp(0);
        DEBUG_DISPATCH(dp);
        dp.update_texture(*this, 0, 0, w, h, (void *)data);
    }
}
void RenderResource::alloc_vertex(u32 stride, u32 vertex_cnt,
                                  const void *data) {
    this->type = RenderResourceType::VERTEX;
    RenderEngine::get_instance()->get_device()->alloc_vertex(this, stride,
                                                             vertex_cnt);
    if (data) {
        RenderCommandDispatcher dp(0);
        DEBUG_DISPATCH(dp);
        dp.update_buffer(*this, 0, stride * vertex_cnt, (void *)data);
    }
}
void RenderResource::alloc_shader(const std::string &vertex_code,
                                  const std::string &fragment_code,
                                  const std::string &geometry_code,
                                  const std::string &tess_ctrl_code,
                                  const std::string &tess_eval_code) {
    this->type = RenderResourceType::SHADER;
    RenderEngine::get_instance()->get_device()->alloc_shader(
        this, vertex_code, fragment_code, geometry_code, tess_ctrl_code,
        tess_eval_code);
}

void RenderResource::alloc_index(const std::vector<u8> &indices) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_BYTE, indices.size());
    RenderCommandDispatcher dp(0);
    DEBUG_DISPATCH(dp);
    dp.update_buffer(*this, 0, indices.size(), (void *)indices.data());
}
void RenderResource::alloc_index(const std::vector<u16> &indices) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_SHORT, indices.size());
    RenderCommandDispatcher dp(0);
    DEBUG_DISPATCH(dp);
    dp.update_buffer(*this, 0, indices.size() * sizeof(u16),
                     (void *)indices.data());
}
void RenderResource::alloc_index(const std::vector<u32> &indices) {
    this->type = RenderResourceType::INDEX;
    RenderEngine::get_instance()->get_device()->alloc_indices(
        this, IndexType::UNSIGNED_INT, indices.size());
    RenderCommandDispatcher dp(0);
    DEBUG_DISPATCH(dp);
    dp.update_buffer(*this, 0, indices.size() * sizeof(u32),
                     (void *)indices.data());
}

void RenderResource::alloc_constant(const std::string &name, u32 size,
                                    void *data) {
    this->type = RenderResourceType::CONSTANT;
    RenderEngine::get_instance()->get_device()->alloc_constant(this, name,
                                                               size);
    if (data) {
        RenderCommandDispatcher dp(0);
        DEBUG_DISPATCH(dp);
        dp.update_buffer(*this, 0, size, (void *)data);
    }
}
void RenderResource::alloc_pipeline(RenderResource shader,
                                    const RenderRasterizerState &rst_state,
                                    const RenderDepthStencilState &depth_state,
                                    const RenderBlendState &blend_state) {
    this->type = RenderResourceType::PIPELINE;
    RenderEngine::get_instance()->get_device()->alloc_pipeline(
        this, shader, rst_state, depth_state, blend_state);
}
void RenderResource::alloc_render_target() {
    this->type = RenderResourceType::RENDER_TARGET;
    RenderEngine::get_instance()->get_device()->alloc_render_target(this);
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