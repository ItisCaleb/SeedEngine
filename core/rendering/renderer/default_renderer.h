#ifndef _SEED_DEFAULT_RENDERER_H_
#define _SEED_DEFAULT_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/model.h"
#include "core/rendering/render_target.h"
#include "core/rendering/shadow_map.h"
#include <unordered_map>

namespace Seed {
class DefaultRenderer : public Renderer {
        friend RenderEngine;

    private:
        std::unordered_map<Model *, std::vector<Mat4>> model_instances;
        VertexLayout instance_desc;
        Ref<Material> debug_mat;
        VertexData sky_vert;
        RenderResource terrain_m;
        VertexData debug_line;
        VertexData debug_triangle;

        /* Lighting */
        ShadowMap shadow_map;
        Ref<RenderTarget> shadow_map_rt;
        RenderResource u_lights;
        RenderResource u_lightspaces;
        RenderResource shadow_map_default_pipeline;
        RenderResource shadow_map_terrain_pipeline;
        Handle shadow_map_dir_handle;

        /* for debugging */

        std::vector<AABB> entity_aabb;
        VertexData aabb_vertices;
        VertexLayout aabb_desc;

        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif