#include "shader.h"
#include <spdlog/spdlog.h>

namespace Seed {

Shader::Shader(const std::string &path, const std::string &code) {
    handle = RHI::alloc_shader(path, code, &this->layout);
}
}  // namespace Seed