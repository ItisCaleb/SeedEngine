#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include "core/io/path.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/types.h"

namespace Seed {

class Image;
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
        void blit_to(Ref<Texture> dst);

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

class MappableTexture : public Texture {
    public:
        MappableTexture(TextureType type, u32 w, u32 h, PixelFormat format,
                        const u8 *image_data);
        MappableTexture(TextureType type, u32 w, u32 h, PixelFormat format,
                        const u8 *image_data, const SamplerProperty &property);
        void update(const u8 *data, u32 w, u32 h);
        void *get_mapped();
        u8 *pixel(u32 x, u32 y) {
            u8 *data = (u8 *)get_mapped();
            return &data[(y * real_w + x) * get_pixel_format_size(format)];
        }
        u8 *pixel_repeat(i32 x, i32 y) {
            if (x < 0) x = 0;
            if (x >= w) x = w - 1;
            if (y < 0) y = 0;
            if (y >= h) y = h - 1;
            u8 *data = (u8 *)get_mapped();
            return &data[(y * real_w + x) * get_pixel_format_size(format)];
        }

        void save_disk(const Path &path);
};

class TextureArray : public Texture {
    private:
        u32 layers;

    public:
        void update_layer(u32 w, u32 h, u32 layer, const void *data);
        void update_layer(u32 layer, RHI::UpdateBufferInfo info);

        TextureArray(TextureType type, u32 w, u32 h, u32 layers,
                     PixelFormat format)
            : TextureArray(type, w, h, layers, format, SamplerProperty{}) {};
        TextureArray(TextureType type, u32 w, u32 h, u32 layers,
                     PixelFormat format, const SamplerProperty &property);
};
enum class CubemapFace : u8 { RIGHT = 0, LEFT, TOP, BOTTOM, FRONT, BACK };

class TextureCubemap : public Texture {
    public:
        void update_face(u32 w, u32 h, CubemapFace face, const void *data);
        void update_face(u32 w, u32 h, CubemapFace face, Ref<Image> data);

        TextureCubemap(u32 w, u32 h, PixelFormat format)
            : TextureCubemap(w, h, format, SamplerProperty{}) {};
        TextureCubemap(u32 w, u32 h, PixelFormat format,
                       const SamplerProperty &property);
};

}  // namespace Seed

#endif