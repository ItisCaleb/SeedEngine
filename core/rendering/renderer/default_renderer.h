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
class RenderEngine;

class DefaultRenderer : public Renderer {
        friend RenderEngine;

    protected:
        inline static const u32 CSM_SPLITS = 4;

        /*
         * Instance data remains in its type-specific pool. The per-frame
         * visible buffer is an indirection table containing absolute pool
         * element indices selected by frustum culling.
         *
         * Each mesh records its contiguous slice in that table. Shaders use
         * visible_offset + instance ID to resolve the actual pool element.
         */
        struct MeshInstance {
                Ref<Mesh> mesh;
                /* First entry of this mesh's camera-visible slice. */
                u32 visible_offset;
                /* Number of entries in the camera-visible slice. */
                u32 visible_size;
                /* Depths in the same order as the visible entries. */
                std::vector<f32> depth;
        };

        struct ShadowMeshInstance {
                Ref<Mesh> mesh;
                /* One visible-buffer slice per CSM split. */
                std::vector<u32> visible_offset;
                std::vector<u32> visible_size;
                /* Culling depths appended in CSM split order. */
                std::vector<f32> depth;
        };
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
                void setup(RenderPass<FrameData> &renderpass) {
                    this->set_name("Debug Pass");
                    this->set_renderpass(renderpass);
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
        void _process(RenderCommandDispatcher &dp) override;
        void cleanup() override;
};
}  // namespace Seed

#endif
