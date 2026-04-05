#include "shader.h"
#include <spdlog/spdlog.h>

namespace Seed {

Shader::Shader(const Path &path, const std::string &code) {
    handle = RHI::alloc_shader(path, code, &this->layout);
}
}  // namespace Seed