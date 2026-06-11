#include "shader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource_loader.h"

namespace Seed {

Shader::Shader(const Path &path, const KString &code)
    : Shader(path, code, {}) {}

Shader::Shader(const Path &path, const KString &code,
               const std::vector<ShaderDefine> &defines)
    : defines(defines) {
    handle = RHI::alloc_shader(path, code, &this->layout, defines);
    u32 param_size = this->layout.get_ubo_binding().total_size;
    if (param_size > 0) {
        param_handle =
            RHI::alloc_constant(param_size * 100, UpdateFrequence::PERDRAW);
    }
}

bool Shader::reload_from_disk() {
    ResourceEntry *entry =
        ResourceLoader::get_instance()->get_entries().get_entry(get_uuid());
    if (!entry) {
        SPDLOG_ERROR("Can't reload shader, this shader is not from disk.");
        return false;
    }
    Path path = entry->real_path();
    Ref<File> data = File::open(path);
    KString shader_code;
    shader_code = data->read_str();

    ShaderHandle new_handle =
        RHI::alloc_shader(path, shader_code, &this->layout, defines);
    if (new_handle == NULL_HANDLE) return false;

    RHI::dealloc(handle);
    if (param_handle != NULL_HANDLE) RHI::dealloc(param_handle);
    u32 param_size = this->layout.get_ubo_binding().total_size;
    if (param_size > 0) {
        param_handle =
            RHI::alloc_constant(param_size * 100, UpdateFrequence::PERDRAW);
    }
    handle = new_handle;
    version++;
    return true;
}
Ref<Shader> Shader::create_variant(const std::vector<ShaderDefine> &defines) {
    ResourceEntry *entry =
        ResourceLoader::get_instance()->get_entries().get_entry(get_uuid());
    if (!entry) {
        SPDLOG_ERROR("Can't reload shader, this shader is not from disk.");
        return Ref<Shader>();
    }
    Path path = entry->real_path();
    Ref<File> data = File::open(path);
    KString shader_code;
    shader_code = data->read_str();
    Ref<Shader> new_shader;
    new_shader.create(path, shader_code, defines);
    /* set same uuid to allow reload from path */
    new_shader->set_uuid(get_uuid());
    return new_shader;
}

Shader::~Shader() {
    RHI::dealloc(handle);
    if (param_handle != NULL_HANDLE) RHI::dealloc(param_handle);
}

}  // namespace Seed