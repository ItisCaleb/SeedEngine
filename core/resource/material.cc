#include "material.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <cstring>
#include "core/handle.h"
#include "core/macro.h"
#include "core/rendering/rhi/render_command.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/rendering/shader_layout.h"
#include "core/types.h"
#include "shader.h"

namespace Seed {

Material::Material(Ref<Shader> shader, const RenderRasterizerState &rst_state,
                   const RenderDepthStencilState &depth_state,
                   const RenderBlendState &blend_state)
    : id(last_id++),
      shader(shader),
      raster_state(rst_state),
      depth_state(depth_state),
      blend_state(blend_state) {
    last_shader_version = shader->get_version();
    shadow_map_unit = shader->get_layout().get_texture_unit("shadowMap");
    param_size = shader->get_layout().get_ubo_binding().total_size;
    if (param_size > 0) {
        parameters = malloc(param_size);
        memset(parameters, 0, param_size);
    }
}

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

void Material::set_parameter(const std::string &name, const void *value,
                             u32 size) {
    const UBOBinding &ubo = shader->get_layout().get_ubo_binding();
    auto iter = ubo.members.find(name);
    if (iter == ubo.members.end()) {
        SPDLOG_WARN("Can't find variable '{}' in shader '{}'", name,
                    shader->get_path());
        return;
    }
    if (iter->second.size != size) {
        SPDLOG_WARN("Size of variable '{}' doesn't match", name);
        return;
    }
    memcpy((void *)((u64)parameters + iter->second.offset), value, size);
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
    if (shader->get_version() != last_shader_version ||
        pipeline.handle == NULL_HANDLE) {
        last_shader_version = shader->get_version();
        this->pipeline = RHI::alloc_pipeline(shader->get_handle(), raster_state,
                                             depth_state, blend_state);
    }
    return this->pipeline;
}

void Material::upload_parameter(RenderCommandDispatcher &dp) {
    ConstantHandle handle = shader->get_param_handle();
    if (handle != NULL_HANDLE) {
        dp.push_buffer(handle, param_size, parameters);
    }
}

void Material::bind_states(RenderDrawDataBuilder &builder) {
    for (auto &iter : textures) {
        Ref<Texture> tex = iter.second;
        if (tex.is_valid()) {
            builder.bind_texture(iter.first, tex->get_handle());
        }
    }
    ConstantHandle constant_handle = shader->get_param_handle();
    if (constant_handle != NULL_HANDLE) {
        builder.bind_constant(shader->get_layout().get_ubo_binding().binding,
                              constant_handle);
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
