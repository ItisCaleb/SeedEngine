#include "shader.h"
#include <spdlog/spdlog.h>
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"

namespace Seed {

Shader::Shader(const Path &path, const std::string &code) {
    handle = RHI::alloc_shader(path, code, &this->layout);
    u32 param_size = this->layout.get_ubo_binding().total_size;
    if (param_size > 0) {
        param_handle =
            RHI::alloc_constant(param_size * 100, UpdateFrequence::PERDRAW);
    }
}
}  // namespace Seed