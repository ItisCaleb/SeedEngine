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
        inline static const u32 CSM_SPLITS = 4;

        struct MeshInstance {
                Ref<Mesh> mesh;
                std::vector<u32> instance_id;
                std::vector<f32> depth;
                bool instance;
        };

        struct ShadowMeshInstance {
                Ref<Mesh> mesh;
                std::vector<u32> instance_id;
                std::vector<f32> depth;
                std::vector<u32> instance_ranges;
        };
        std::vector<MeshInstance> opaque_meshes;
        std::vector<MeshInstance> transparent_meshes;

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
        RenderResource u_csm;
        Handle shadow_map_dir_handle[CSM_SPLITS];
        std::vector<ShadowMeshInstance> shadow_meshes;

        /* for debugging */

        std::vector<AABB> entity_aabb;
        Ref<VertexData> aabb_vertices;
        VertexLayout aabb_desc;
        void prepare_lights();
        void prepare_meshes();
        void shadow_pass();
        void color_pass(Viewport &viewport);
        void debug_pass(Viewport &viewport);

        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif