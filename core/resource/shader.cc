#include "shader.h"
#include <regex>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include <sstream>
#include "core/io/file.h"
#include <filesystem>

namespace Seed {

static const std::vector<std::string> DEFAULT_INCLUDE_PATHS = {
    "assets/shader/include",
};

std::string Shader::preprocess(const std::string &shader) {
    std::unordered_set<std::string> included;
    static int depth = 0;
    const int MAX_DEPTH = 50;

    if(shader.size() == 0){
        return "";
    }

    if (++depth > MAX_DEPTH) {
        --depth;
        SPDLOG_ERROR("Shader include path too deep.");
        return "";
    }

    std::ostringstream result;
    std::istringstream input(shader);
    std::string line;
    std::regex include_regex(R"(^\s*#\s*include\s*\<([^>]+)\>)");

    while (std::getline(input, line)) {
        std::smatch match;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (std::regex_match(line, match, include_regex)) {
            std::string filename = match[1].str();

            std::filesystem::path found_path;
            for (const auto &dir : DEFAULT_INCLUDE_PATHS) {
                auto full_path = std::filesystem::path(dir) / filename;
                if (std::filesystem::exists(full_path)) {
                    found_path = std::filesystem::weakly_canonical(full_path);
                    break;
                }
            }

            if (found_path.empty()) {
                SPDLOG_ERROR("Not found: #include <{}>", filename);
                continue;
            }

            std::string norm_path = found_path.string();
            if (included.count(norm_path)) {
                SPDLOG_WARN("Already included: #include <{}>", filename);
                continue;
            }
            included.insert(norm_path);

            Ref<File> file = File::open(norm_path, "rb");
            if (file.is_valid()) {
                result << preprocess(file->read_str());
            } else {
                result << line << '\n';
            }
        } else {
            result << line << '\n';
        }
    }

    --depth;
    return result.str();
}

Shader::Shader(const std::string &vertex, const std::string &frag,
               const std::string &geom, const std::string &tesc,
               const std::string &tese) {
    std::string _vertex = preprocess(vertex);
    std::string _frag = preprocess(frag);
    std::string _geom = preprocess(geom);
    std::string _tesc = preprocess(tesc);
    std::string _tese = preprocess(tese);

    shader.alloc_shader(_vertex, _frag, _geom, _tesc, _tese);
}
}  // namespace Seed