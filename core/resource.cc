#include <glad/glad.h>
#include "resource.h"
#include "io/file.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Seed {
ResourceLoader *ResourceLoader::get_instance() { return instance; }

ResourceLoader::ResourceLoader() {
    instance = this;
    spdlog::info("Initializing Resource loader");
}

ResourceLoader::~ResourceLoader() { instance = nullptr; }

RenderResource ResourceLoader::load_texture(const std::string &path) {
    int w, h, comp;
    RenderResource tex_rc;
    stbi_set_flip_vertically_on_load(true);
    void *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
    if (!data) return {};
    tex_rc.alloc_texture(w, h, data);

    stbi_image_free(data);
    return tex_rc;
}

RenderResource ResourceLoader::loadShader(const std::string &vertex_path,
                                          const std::string &fragment_path) {
    RenderResource shader_rc;
    Ref<File> vertex_f = File::open(vertex_path, "r");
    if (vertex_f.is_null()) {
        throw std::exception("Can't open vertex shader.");
    }
    Ref<File> fragment_f = File::open(fragment_path, "r");
    if (fragment_f.is_null()) {
        throw std::exception("Can't open fragment shader.");
    }
    std::string vertex_s = vertex_f->read();
    std::string fragment_s = fragment_f->read();
    const char *vertex_code = vertex_s.c_str();
    const char *fragment_code = fragment_s.c_str();
    try {
        shader_rc.alloc_shader(vertex_code, fragment_code);
        return shader_rc;
    } catch (std::exception &e) {
        throw e;
    }
}

}  // namespace Seed