#include "mappable_texture.h"
#include <spdlog/spdlog.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace Seed {
MappableTexture::MappableTexture(TextureType type, u32 w, u32 h,
                                 PixelFormat format, const u8 *image_data)
    : MappableTexture(type, w, h, format, image_data, SamplerProperty{}) {}
MappableTexture::MappableTexture(TextureType type, u32 w, u32 h,
                                 PixelFormat format, const u8 *image_data,
                                 const SamplerProperty &property) {
    this->type = type;
    this->format = format;
    this->w = w;
    this->h = h;
    this->property = property;
    handle =
        RHI::alloc_mappable_texture(type, w, h, format, image_data, property);
    RHI::query_texture_size(handle, &real_w, &real_h);
}
void MappableTexture::update(const u8 *data, u32 w, u32 h) {
    if (this->w != w || this->h != h) {
        SPDLOG_ERROR("data size is not equal to texture");
        return;
    }
    void *texture_data = RHI::map_texture(handle);
    memcpy(texture_data, data, w * h * get_pixel_format_size(format));
}

void *MappableTexture::get_mapped() { return RHI::map_texture(handle); }

void MappableTexture::save_disk(const std::string &path) {
    void *data = get_mapped();
    u32 pixel_size = get_pixel_format_size(format);
    void *tmp = malloc(w * h * pixel_size);
    if (w == real_w) {
        /* if the layout is same, we can just memcpy */
        memcpy(tmp, data, w * h * pixel_size);
    } else {
        /* else we copy line by line */
        for (u32 i = 0; i < h; i++) {
            memcpy((void *)((u64)tmp + i * w * pixel_size),
                   (void *)((u64)data + i * real_w * pixel_size),
                   w * pixel_size);
        }
    }
    stbi_write_png(path.c_str(), w, h, get_pixel_format_size(format), tmp, 0);
    free(tmp);
}

}  // namespace Seed