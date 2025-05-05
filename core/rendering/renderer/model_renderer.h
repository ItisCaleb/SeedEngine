#ifndef _SEED_MODEL_RENDERER_H_
#define _SEED_MODEL_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include <unordered_map>
#include "core/resource/model.h"

namespace Seed {
class ModelRenderer : public Renderer {
    friend RenderEngine;

   private:
    std::unordered_map<Model *, std::vector<Mat4>> model_instances;
    Ref<Texture> default_texture;
    Ref<Material> default_material;
    VertexDescription vertices_desc;
    VertexDescription instance_desc;
    RenderResource color_shader;
    
    /* for debugging */
    std::vector<AABB> entity_aabb;
    RenderResource aabb_vertices_rc;
    VertexDescription aabb_desc;
    RenderResource debugging_shader;


    void init_color();
    void init_debugging();
    void init() override;
    void preprocess() override;
    void process(RenderCommandDispatcher &dp, u64 sort_key) override;
    void cleanup() override;
};
}  // namespace Seed

#endif