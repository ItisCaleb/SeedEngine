#ifndef _SEED_MAPPABLE_TEXTURE_H_
#define _SEED_MAPPABLE_TEXTURE_H_
#include "core/resource/texture.h"

namespace Seed {
class MappableTexture : public Texture {
    public:
        MappableTexture(TextureType type, u32 w, u32 h, PixelFormat format,
                        const u8 *image_data);
        MappableTexture(TextureType type, u32 w, u32 h, PixelFormat format,
                        const u8 *image_data, const SamplerProperty &property);
        void update(const u8 *data, u32 w, u32 h);
        void upload_cube_map(const u8 *right, const u8 *left, const u8 *top,
                             const u8 *bottom, const u8 *front,
                             const u8 *back) {}
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

        void save_disk(const std::string &path);
};
}  // namespace Seed

#endif