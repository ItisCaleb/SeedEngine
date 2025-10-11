#ifndef _SEED_DEFAULT_RENDERER_H_
#define _SEED_DEFAULT_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/model.h"
#include "core/rendering/render_target.h"
#include "core/rendering/shadow_map.h"
#include <vector>

namespace Seed {
class DefaultRenderer : public Renderer {
        friend RenderEngine;

    private:
        struct MeshInstance{
            Ref<Mesh> mesh;
            std::vector<u32> instance_id;
            std::vector<f32> depth;
        };
        std::vector<MeshInstance> opaque_meshes;
        std::vector<MeshInstance> transparent_meshes;
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
        void shadow_pass();
        void color_pass(Viewport &viewport);

        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif