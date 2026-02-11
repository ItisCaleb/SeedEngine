#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/rendering/api/render_resource.h"
#include "core/resource/resource.h"

namespace Seed {
class Shader : public Resource {
    private:
        RenderResource shader;
        std::string path;
        u8 tex_unit_cnt;

    public:
        Shader(const std::string &path, const std::string &code);
        RenderResource &get_render_resource() { return shader; }
        ~Shader() { shader.dealloc(); }
};
}  // namespace Seed

#endif