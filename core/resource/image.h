#ifndef _SEED_IMAGE_H_
#define _SEED_IMAGE_H_
#include "core/rendering/render_common.h"
#include "core/types.h"
#include "core/resource/resource.h"
#include "core/resource/texture.h"
#include "core/resource/mappable_texture.h"
#include <vector>

namespace Seed {
class Image : public Resource {
    private:
        PixelFormat format;
        std::vector<u8> data;
        u32 width, height;

    public:
        void update(std::vector<u8> &data, u32 w, u32 h, u32 off_x = 0,
                    u32 off_y = 0);
        void update(u8 *data, u32 w, u32 h, u32 off_x = 0, u32 off_y = 0);
        void fill(Color color, u32 w, u32 h, u32 off_x = 0, u32 off_y = 0);
        Ref<Texture> create_texture(const SamplerProperty &property = {});
        Ref<MappableTexture> create_mappable_texture(
            const SamplerProperty &property = {});

        void upload(Ref<Texture> texture);
        void download(Ref<Texture> texture);
        u32 get_width() { return width; }
        u32 get_height() { return height; }
        u8 *pixel(u32 x, u32 y) {
            return &this->data[(y * width + x) * get_pixel_format_size(format)];
        }
        u8 *pixel_repeat(i32 x, i32 y) {
            if (x < 0) x = 0;
            if (x >= width) x = width - 1;
            if (y < 0) y = 0;
            if (y >= height) y = height - 1;
            return &this->data[(y * width + x) * get_pixel_format_size(format)];
        }

        std::vector<u8> &get_data() { return data; }

        Ref<Image> median_filter(u32 kernel_size);

        Image(PixelFormat format, u32 w, u32 h);
};
}  // namespace Seed

#endif