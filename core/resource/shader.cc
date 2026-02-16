#include "shader.h"
#include <regex>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include <sstream>
#include "core/io/file.h"
#include <filesystem>

namespace Seed {

Shader::Shader(const std::string &path, const std::string &code) {
    handle = RHI::alloc_shader(path, code, &this->layout);
}
}  // namespace Seed