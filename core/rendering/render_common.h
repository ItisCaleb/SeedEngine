#ifndef _SEED_RENDER_COMMON_H_
#define _SEED_RENDER_COMMON_H_
#include "core/types.h"

struct Color {
        u8 r, g, b, a;
};

enum class TextureType : u8 {
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBEMAP,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBEMAP_ARRAY,
    TEXTURE_2D_MULTISAMPLE
};

enum class SamplerFilter : u8 { LINEAR, NEAREST };

enum class SamplerWrap : u8 {
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRRORED_REPEAT,
    REPEAT,
    MIRROR_CLAMP_TO_EDGE
};

struct SamplerProperty {
        SamplerFilter min_filter = SamplerFilter::LINEAR;
        SamplerFilter mag_filter = SamplerFilter::LINEAR;
        SamplerWrap wrap_u = SamplerWrap::CLAMP_TO_EDGE;
        SamplerWrap wrap_v = SamplerWrap::CLAMP_TO_EDGE;
        SamplerWrap wrap_w = SamplerWrap::CLAMP_TO_EDGE;
};

enum class PixelFormat : u8 {
    R,
    RG,
    RGB,
    RGBA,
    RGBA16F,
    RGBA16I,
    D24,
    D24S8,
    D32,
    D32S8,
    S8
};

u32 constexpr static get_pixel_format_size(PixelFormat format) {
    switch (format) {
        case PixelFormat::R:
            return 1;
        case PixelFormat::RG:
            return 2;
        case PixelFormat::RGB:
        case PixelFormat::D24:
            return 3;
        case PixelFormat::RGBA:
        case PixelFormat::D24S8:
        case PixelFormat::D32:
            return 4;
        case PixelFormat::D32S8:
            return 5;
        case PixelFormat::RGBA16F:
        case PixelFormat::RGBA16I:
            return 8;
        default:
            return 1;
    }
}

enum class RenderPrimitiveType : u8 { LINES, TRIANGLES, POINTS, PATCHES };
enum class PolygonMode : u8 { POINT, LINE, FILL };
enum class Cullmode : u8 { NONE, FRONT, BACK, BOTH };
enum class CompareOP : u8 {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS
};

struct RenderRasterizerState {
        Cullmode cull_mode = Cullmode::NONE;
        u32 patch_control_points = 1;
        PolygonMode poly_mode = PolygonMode::FILL;
};

struct RenderDepthStencilState {
        bool depth_on = false;
        bool stencil_on = false;
        CompareOP depth_compare_op = CompareOP::LESS;
        CompareOP stencil_compare_op = CompareOP::ALWAYS;
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
        BlendFunc func = {BlendFactor::ONE, BlendFactor::ZERO, BlendFactor::ONE,
                          BlendFactor::ZERO

        };
};

enum class UpdateFrequence : u8 { 
    /* Update occasionally or never*/
    STATIC, 
    /* Update per frame*/
    PERFRAME, 
    /* Update per drawcall*/
    PERDRAW };

#endif