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
            bool instance;
        };
        std::vector<MeshInstance> opaque_meshes;
        std::vector<MeshInstance> transparent_meshes;
        VertexLayout instance_desc;
        RenderResource instance_idx_rc;

        Ref<Material> debug_mat;
        Ref<VertexData> sky_vert;
        RenderResource terrain_m;
        Ref<VertexData> debug_line;
        Ref<VertexData> debug_triangle;
        Ref<IndexData> debug_triangle_indices;

        /* Lighting */
        ShadowMap shadow_map;
        Ref<RenderTarget> shadow_map_rt;
        RenderResource u_lights;
        RenderResource u_lightspaces;
        inline static const u32 CSM_SPLITS = 4;
        Handle shadow_map_dir_handle[CSM_SPLITS];
        std::vector<MeshInstance> shadow_meshes[CSM_SPLITS];


        /* for debugging */

        std::vector<AABB> entity_aabb;
        Ref<VertexData> aabb_vertices;
        VertexLayout aabb_desc;
        void shadow_pass();
        void color_pass(WindowViewport &viewport);

        void init() override;
        void preprocess() override;
        void process(WindowViewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif