#ifndef _SEED_DEFAULT_RENDERER_H_
#define _SEED_DEFAULT_RENDERER_H_
#include "renderer.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/model.h"
#include "core/rendering/render_pass.h"
#include "core/rendering/shadow_map.h"
#include <vector>

namespace Seed {
class DefaultRenderer : public Renderer {
        friend RenderEngine;

    public:
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

        /* Lighting */
        SSBOHandle visible_ssbo;
        SSBOHandle transform_ssbo;
        SSBOHandle terrain_ssbo;
        SSBOHandle bone_ssbo;
        ConstantHandle mvp;
        ConstantHandle camera;
        ConstantHandle u_lights;
        ConstantHandle u_csm;
        /* for debugging */

        std::vector<AABB> entity_aabb;
        Ref<VertexData> aabb_vertices;
        VertexLayout aabb_desc;

        struct FrameData {
                ShadowMap shadow_map;
                Handle shadow_map_dir_handle[CSM_SPLITS];
                std::vector<ShadowMeshInstance> shadow_meshes;
                std::vector<MeshInstance> opaque_meshes;
                std::vector<MeshInstance> transparent_meshes;
                Ref<Material> post_mat;
                Ref<Material> debug_mat;
                Ref<VertexData> sky_vert;
                Ref<VertexData> debug_line;
                Ref<VertexData> debug_triangle;
                Ref<IndexData> debug_triangle_indices;
        } fd;

        class ShadowPass : public RenderPass<FrameData> {
            public:
                void setup(ShadowMap &shadowmap) {
                    this->set_name("Shadow Pass");
                    u32 rs = shadowmap.get_resolution();
                    this->set_viewport(Viewport(rs, rs));
                    this->bind_depth_attachment(shadowmap.get_texture(), 0);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };
        class ColorPass : public RenderPass<FrameData> {
            public:
                void setup(Ref<Texture> color, Ref<Texture> depth) {
                    this->set_name("Color Pass");
                    this->set_viewport(
                        Viewport(color->get_width(), color->get_height()));
                    this->bind_color_attachment(color, 0, 0);
                    this->bind_depth_attachment(depth, 0);
                    this->set_clear_flag(CLEAR_COLOR | CLEAR_DEPTH);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };
        class DebugPass : public RenderPass<FrameData> {
            public:
                void setup(Ref<Texture> color, Ref<Texture> depth) {
                    this->set_name("Debug Pass");
                    this->set_viewport(
                        Viewport(color->get_width(), color->get_height()));
                    this->bind_color_attachment(color, 0, 0);
                    this->bind_depth_attachment(depth, 0);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };
        class PostPass : public WindowRenderPass<FrameData> {
            public:
                void setup(Window *window) {
                    this->set_name("Post Pass");
                    this->set_window(window);
                    this->set_clear_flag(CLEAR_COLOR);
                }
                void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             FrameData &fd) override;
        };
        ShadowPass shadow_pass;
        ColorPass color_pass;
        DebugPass debug_pass;
        PostPass post_pass;
        void prepare_lights();
        void prepare_meshes();

        void init(Window *window) override;
        void preprocess() override;
        void process() override;
        void cleanup() override;
};
}  // namespace Seed

#endif