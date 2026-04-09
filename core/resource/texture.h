#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/types.h"

namespace Seed {

class Texture : public Resource {
    protected:
        TextureHandle handle;
        TextureType type;
        PixelFormat format;
        SamplerProperty property;
        bool mipmap;
        u32 w, h;
        /* real width on GPU */
        u32 real_w, real_h;
        Texture() {}

    public:
        TextureHandle get_handle() { return handle; }
        u32 get_width() { return w; }

        u32 get_height() { return h; }

        void update(const u8 *data, u32 w, u32 h);
        void update_sampler(const SamplerProperty &property);

        Texture(TextureType type, u32 w, u32 h, PixelFormat format);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                const u8 *image_data);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                const SamplerProperty &property);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                const SamplerProperty &property, const u8 *image_data);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                MSAAType msaa_type, const SamplerProperty &property,
                const u8 *image_data);

        ~Texture();
};

class TextureArray : public Texture {
    public:
        void update_layer(u32 w, u32 h, u32 layer, const void *data);
        TextureArray(TextureType type, u32 w, u32 h, u32 layers,
                     PixelFormat format, const SamplerProperty &property);
};
enum class CubemapFace : u8 { RIGHT = 0, LEFT, TOP, BOTTOM, FRONT, BACK };

class TextureCubemap : public Texture {
    public:
        void update_face(u32 w, u32 h, CubemapFace face, const void *data);
        TextureCubemap(u32 w, u32 h, PixelFormat format,
                       const SamplerProperty &property);
};

}  // namespace Seed

#endif