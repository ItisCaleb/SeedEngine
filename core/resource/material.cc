#include "material.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include "core/rendering/api/render_command.h"

namespace Seed {

void Material::set_texture_unit(u32 unit, Ref<Texture> texture) {
    EXPECT_INDEX_INBOUND(unit, this->textures.size());
    this->textures[unit].bind_texture(texture);
}
void Material::add_texture_unit(Ref<Texture> tex) {
    this->textures.push_back(TextureState(tex));
}
void Material::remove_texture_unit(u32 unit) {
    EXPECT_INDEX_INBOUND(unit, this->textures.size());
    this->textures.erase(this->textures.begin() + unit);
}

void Material::set_rasterizer_state(RenderRasterizerState &state) {
    this->raster_state = state;
    this->pipeline_dirty = true;
}
void Material::set_depth_state(RenderDepthStencilState &state) {
    this->depth_state = state;
    this->pipeline_dirty = true;
}
void Material::set_blend_state(RenderBlendState &state) {
    this->blend_state = state;
    this->pipeline_dirty = true;
}
void Material::set_shader(Ref<Shader> shader) {
    this->shader = shader;
    this->pipeline_dirty = true;
}

void Material::build_pipeline() {
    EXPECT_NOT_NULL_RET(shader.ptr());
    if (this->pipeline.inited()) {
        this->pipeline.dealloc();
    }
    this->pipeline.alloc_pipeline(shader->get_render_resource(), raster_state,
                                  depth_state, blend_state);
}

RenderResource Material::get_pipeline() {
    if (pipeline_dirty) {
        build_pipeline();
        pipeline_dirty = false;
    }
    return this->pipeline;
}

void Material::bind_states(RenderDrawDataBuilder &builder) {
    for (i32 i = 0; i < this->textures.size(); i++) {
        Ref<Texture> tex = textures[i].get_texture();
        if (tex.is_valid()) {
            builder.bind_texture(i, tex->get_render_resource());
        }
    }
}

TextureState *Material::get_texture_unit(u32 unit) {
    EXPECT_INDEX_INBOUND_RET(unit, this->textures.size(), nullptr);
    return &this->textures[unit];
}

}  // namespace Seed
