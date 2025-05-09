#ifndef _SEED_MODEL_RENDERER_H_
#define _SEED_MODEL_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_data.h"
#include <unordered_map>
#include "core/resource/model.h"

namespace Seed {
class ModelRenderer : public Renderer {
    friend RenderEngine;

   private:
    std::unordered_map<Model *, std::vector<Mat4>> model_instances;
    VertexDescription vertices_desc;
    VertexDescription instance_desc;
    RenderResource color_shader;
    
    /* for debugging */
    std::vector<AABB> entity_aabb;
    VertexData aabb_vertices;
    VertexDescription aabb_desc;
    RenderResource debugging_shader;


    void init_color();
    void init_debugging();
    void init() override;
    void preprocess() override;
    void process(u8 layer) override;
    void cleanup() override;
};
}  // namespace Seed

#endif