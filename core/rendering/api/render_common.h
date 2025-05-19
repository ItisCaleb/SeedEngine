#ifndef _SEED_RENDER_COMMON_H_
#define _SEED_RENDER_COMMON_H_

enum class TextureType {
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_2D_ARRAY
};

enum class TextureFormat { FORMAT_R, FORMAT_RG, FORMAT_RGB, FORMAT_RGBA };

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

#endif