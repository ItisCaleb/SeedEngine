#ifndef _SEED_RENDER_PIPELINE_H_
#define _SEED_RENDER_PIPELINE_H_
#include "core/resource/shader.h"
#include "core/rendering/vertex_desc.h"
#include "core/ref.h"

namespace Seed {

enum class RenderPrimitiveType { LINES, TRIANGLES, POINTS, PATCHES };
enum class PolygonMode { POINT, LINE, FILL };
enum class Cullmode { NONE, FRONT, BACK, BOTH };

struct RenderRasterizerState {
        Cullmode cull_mode = Cullmode::NONE;
        u32 patch_control_points = 1;
        PolygonMode poly_mode = PolygonMode::FILL;
};

struct RenderDepthStencilState {
        bool depth_on = false;
        bool stencil_on = false;
};
enum class BlendFactor {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
    SRC1_COLOR,
    ONE_MINUS_SRC1_COLOR,
    SRC1_ALPHA,
    ONE_MINUS_SRC1_ALPHA
};
struct BlendFunc {
        BlendFactor src_rgb;
        BlendFactor dst_rgb;
        BlendFactor src_alpha;
        BlendFactor dst_alpha;
        inline static BlendFunc create(
            BlendFactor src_rgb = BlendFactor::ONE,
            BlendFactor dst_rgb = BlendFactor::ZERO,
            BlendFactor src_alpha = BlendFactor::ONE,
            BlendFactor dst_alpha = BlendFactor::ZERO) {
            return BlendFunc{.src_rgb = src_rgb,
                             .dst_rgb = dst_rgb,
                             .src_alpha = src_alpha,
                             .dst_alpha = dst_alpha};
        }
};

struct RenderBlendState {
        bool blend_on = false;
        BlendFunc func;
};

struct RenderPipeline : public RefCounted {
        Ref<Shader> shader;
        VertexDescription *desc;
        RenderPrimitiveType type;
        RenderRasterizerState rst_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_state;
        VertexDescription *instance_desc;

        RenderPipeline(Ref<Shader> shader, VertexDescription *desc,
                       RenderPrimitiveType type,
                       const RenderRasterizerState &rst_state,
                       const RenderDepthStencilState &depth_state,
                       const RenderBlendState &blend_state,
                       VertexDescription *instance_desc = nullptr);
};

}  // namespace Seed
#endif