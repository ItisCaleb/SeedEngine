#ifndef _SEED_DEFAULT_RENDERER_H_
#define _SEED_DEFAULT_RENDERER_H_
#include "renderer.h"
#include "core/rendering/rhi/render_resource.h"
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
                u32 visible_offset;
                u32 visible_size;
                std::vector<f32> depth;
        };

        struct ShadowMeshInstance {
                Ref<Mesh> mesh;
                std::vector<u32> visible_offset;
                std::vector<u32> visible_size;
                std::vector<f32> depth;
        };
        std::vector<MeshInstance> opaque_meshes;
        std::vector<MeshInstance> transparent_meshes;
        std::vector<u32> visible_instances;

        Ref<Material> post_mat;
        Ref<Material> debug_mat;
        Ref<VertexData> sky_vert;
        Ref<VertexData> debug_line;
        Ref<VertexData> debug_triangle;
        Ref<IndexData> debug_triangle_indices;

        /* Lighting */
        ShadowMap shadow_map;
        Ref<RenderTarget> shadow_map_rt;
        ConstantHandle mvp;
        ConstantHandle camera;
        ConstantHandle u_lights;
        ConstantHandle u_csm;
        Handle shadow_map_dir_handle[CSM_SPLITS];
        std::vector<ShadowMeshInstance> shadow_meshes;

        /* for debugging */

        std::vector<AABB> entity_aabb;
        Ref<VertexData> aabb_vertices;
        VertexLayout aabb_desc;
        void prepare_lights();
        void prepare_meshes();
        void depth_prepass(Viewport &viewport);
        void shadow_pass();
        void color_pass(Viewport &viewport);
        void debug_pass(Viewport &viewport);
        void post_pass();

        void init() override;
        void preprocess() override;
        void process(Viewport &viewport) override;
        void cleanup() override;
};
}  // namespace Seed

#endif