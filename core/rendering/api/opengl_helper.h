#ifndef _SEED_OPENGL_HELPER_H_
#define _SEED_OPENGL_HELPER_H_
#include <glad/glad.h>
#include "core/rendering/render_common.h"
#include <spdlog/spdlog.h>
#include <regex>
namespace Seed {

class GLHelper {
    private:
        inline static const GLenum sblend_factor[] = {
            GL_ZERO,
            GL_ONE,
            GL_SRC_COLOR,
            GL_ONE_MINUS_SRC_COLOR,
            GL_DST_COLOR,
            GL_ONE_MINUS_DST_COLOR,
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA,
            GL_DST_ALPHA,
            GL_ONE_MINUS_DST_ALPHA,
            GL_CONSTANT_COLOR,
            GL_ONE_MINUS_CONSTANT_COLOR,
            GL_CONSTANT_ALPHA,
            GL_ONE_MINUS_CONSTANT_ALPHA,
            GL_SRC_ALPHA_SATURATE,
            GL_SRC1_COLOR,
            GL_ONE_MINUS_SRC1_COLOR,
            GL_SRC1_ALPHA,
            GL_ONE_MINUS_SRC1_ALPHA};
        inline static const GLenum scompare_op[] = {
            GL_NEVER,   GL_LESS,     GL_EQUAL,  GL_LEQUAL,
            GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS,
        };

        inline static const GLenum stexture_type[] = {
            GL_TEXTURE_1D,
            GL_TEXTURE_2D,
            GL_TEXTURE_3D,
            GL_TEXTURE_CUBE_MAP,
            GL_TEXTURE_1D_ARRAY,
            GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_CUBE_MAP_ARRAY,
            GL_TEXTURE_2D_MULTISAMPLE};

        inline static const GLenum spixel_internal[] = {GL_R8,
                                                        GL_RG,
                                                        GL_RGB,
                                                        GL_RGBA,
                                                        GL_RGBA16F,
                                                        GL_DEPTH_COMPONENT24,
                                                        GL_DEPTH24_STENCIL8};
        inline static const GLenum spixel_format[] = {
            GL_RED,          GL_RG,   GL_RGB,
            GL_RGBA,         GL_RGBA, GL_DEPTH_COMPONENT,
            GL_DEPTH_STENCIL};

        inline static const GLenum ssampler_wrap[] = {
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT, GL_REPEAT,
            GL_MIRROR_CLAMP_TO_EDGE};

    public:
        inline static GLenum blend_factor(BlendFactor factor) {
            return sblend_factor[(u8)factor];
        }

        inline static GLenum compare_op(CompareOP op) {
            return scompare_op[(u8)op];
        }

        inline static GLenum texture_type(TextureType type) {
            return stexture_type[(u8)type];
        }

        inline static GLenum pixel_internal(PixelFormat format) {
            return spixel_internal[(u8)format];
        }

        inline static GLenum pixel_format(PixelFormat format) {
            return spixel_format[(u8)format];
        }

        inline static GLenum wrap_mode(SamplerWrap wrap) {
            return ssampler_wrap[(u8)wrap];
        }

        inline static GLenum filter(SamplerFilter filter) {
            return filter == SamplerFilter::LINEAR ? GL_LINEAR : GL_NEAREST;
        }

        static void find_samplers(const std::string &src,
                                  std::vector<std::string> &result) {
            std::regex sampler_regex(
                R"(\buniform\s+sampler\w*\s+(\w+)(\s*\[\s*(\d+)\s*\])?)");
            std::smatch match;

            std::string::const_iterator search_start(src.cbegin());
            while (std::regex_search(search_start, src.cend(), match,
                                     sampler_regex)) {
                /* retrive name */
                std::string name = match[1];
                /* check if is array */
                std::string array_size_str = match[3];

                if (!array_size_str.empty()) {
                    i32 array_size = std::stoi(array_size_str);
                    for (i32 i = 0; i < array_size; ++i) {
                        result.push_back(fmt::format("{}[{}]", name, i));
                    }
                } else {
                    result.push_back(name);
                }
                search_start = match.suffix().first;
            }
        }
};

}  // namespace Seed

#endif