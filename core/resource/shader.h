#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/rendering/api/render_resource.h"
#include "core/resource/resource.h"
#include "core/rendering/shader_layout.h"

namespace Seed {
class Shader : public Resource {
    private:
        RenderResource shader;
        ShaderLayout layout;
        std::string path;

    public:
        Shader(const std::string &path, const std::string &code);
        RenderResource &get_render_resource() { return shader; }
        ShaderLayout &get_layout() { return layout; }
        ~Shader() { shader.dealloc(); }
};
}  // namespace Seed

#endif