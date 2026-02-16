#include "material.h"
#include <spdlog/spdlog.h>
#include "core/macro.h"
#include "core/rendering/api/render_command.h"

namespace Seed {

void Material::set_texture_unit(u32 unit, Ref<Texture> texture) {
    this->textures[unit] = texture;
}

void Material::set_texture(const std::string &name, Ref<Texture> texture) {
    u32 unit = shader->get_layout().get_texture_unit(name);
    if (unit == -1) {
        SPDLOG_WARN("Texture name '{}' is not in shader {}", name,
                    shader->get_path());
        return;
    }
    set_texture_unit(unit, texture);
}

void Material::remove_texture_unit(u32 unit) { this->textures.erase(unit); }

void Material::remove_texture(const std::string &name) {
    u32 unit = shader->get_layout().get_texture_unit(name);
    if (unit == -1) {
        SPDLOG_WARN("Texture name '{}' is not in shader {}", name,
                    shader->get_path());
        return;
    }
    remove_texture_unit(unit);
}

PipelineHandle Material::get_pipeline() {
    if (pipeline.handle == NULL_HANDLE) {
        this->pipeline = RHI::alloc_pipeline(shader->get_handle(), raster_state,
                                             depth_state, blend_state);
    }
    return this->pipeline;
}

void Material::bind_states(RenderDrawDataBuilder &builder) {
    for (auto &iter : textures) {
        Ref<Texture> tex = iter.second;
        if (tex.is_valid()) {
            builder.bind_texture(iter.first, tex->get_handle());
        }
    }
}

Ref<Texture> Material::get_texture_unit(u32 unit) {
    return this->textures[unit];
}

Ref<Texture> Material::get_texture(const std::string &name) {
    u32 unit = shader->get_layout().get_texture_unit(name);
    if (unit == -1) {
        SPDLOG_WARN("Texture name '{}' is not in shader {}", name,
                    shader->get_path());
        return Ref<Texture>();
    }
    return get_texture_unit(unit);
}

}  // namespace Seed
