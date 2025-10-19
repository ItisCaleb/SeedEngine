#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/rendering/api/render_resource.h"
#include "core/resource/resource.h"

namespace Seed {
class Shader : public Resource {
    private:
        RenderResource shader;
        u8 tex_unit_cnt;

    public:
        Shader(const std::string &vertex, const std::string &frag,
               const std::string &geom = "", const std::string &tesc = "",
               const std::string &tese = "") {
            const std::string prepend_shader = R"(#version 450 core
                layout(std430, binding = 0) buffer TransformInstanceDatas
                {
                    mat4 b_transform[];
                };
                layout(std430, binding = 1) buffer TerrainInstanceDatas
                {
                    vec4 b_terrain[];
                };
            )";
            std::string _vertex = prepend_shader + vertex;
            std::string _frag = prepend_shader + frag;
            std::string _geom = geom;
            std::string _tesc = tesc;
            std::string _tese = tese;
            if(!geom.empty()) _geom = prepend_shader + geom;
            if(!tesc.empty()) _tesc = prepend_shader + tesc;
            if(!tese.empty()) _tese = prepend_shader + tese;

            shader.alloc_shader(_vertex, _frag, _geom, _tesc, _tese);
        }
        RenderResource &get_render_resource() { return shader; }
        ~Shader() { shader.dealloc(); }
};
}  // namespace Seed

#endif