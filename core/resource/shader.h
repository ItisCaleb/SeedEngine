#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/rendering/api/render_resource.h"
#include "core/resource/resource.h"

namespace Seed {
class Shader : public Resource {
    private:
        RenderResource shader;

    public:
        Shader(const std::string &vertex, const std::string &frag,
               const std::string &geom = "", const std::string &tesc = "",
               const std::string &tese = "") {
            shader.alloc_shader(vertex, frag, geom, tesc, tese);
        }
        RenderResource &ger_render_resource(){
            return shader;
        }
        ~Shader(){
            shader.dealloc();
        }
};
}  // namespace Seed

#endif