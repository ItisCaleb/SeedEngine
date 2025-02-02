#ifndef _SEED_COLOR_PASS_H_
#define _SEED_COLOR_PASS_H_
#include "render_pass.h"
#include "core/rendering/api/render_resource.h"
#include <unordered_map>
#include "core/rendering/model.h"

namespace Seed {
class ColorPass : public RenderPass {
    friend RenderEngine;

   private:
    std::unordered_map<Model *, std::map<u32, std::vector<Mat4>>> model_instances;
    Ref<Material> default_material;
    RenderResource vertices_desc_rc;
    RenderResource instance_desc_rc;
    RenderResource color_shader;
    
    /* for debugging */
    std::vector<AABB> entity_aabb;
    RenderResource aabb_vertices_rc;
    RenderResource aabb_desc_rc;
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