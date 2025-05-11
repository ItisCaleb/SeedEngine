#ifndef _SEED_MODEL_RENDERER_H_
#define _SEED_MODEL_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_data.h"
#include <unordered_map>
#include "core/resource/model.h"
#include "core/rendering/api/render_pipeline.h"

namespace Seed {
class ModelRenderer : public Renderer {
    friend RenderEngine;

   private:
    std::unordered_map<Model *, std::vector<Mat4>> model_instances;
    VertexDescription color_desc;
    VertexDescription instance_desc;


    Ref<RenderPipeline> color_pipeline;
    
    /* for debugging */
    Ref<RenderPipeline> debug_pipeline;

    std::vector<AABB> entity_aabb;
    VertexData aabb_vertices;
    VertexDescription aabb_desc;


    void init_color();
    void init_debugging();
    void init() override;
    void preprocess() override;
    void process() override;
    void cleanup() override;
    ModelRenderer(u8 layer):Renderer(layer){}
};
}  // namespace Seed

#endif