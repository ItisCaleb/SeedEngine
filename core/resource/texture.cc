#include "texture.h"
#include <spdlog/spdlog.h>
#include "core/io/path.h"
#include "core/macro.h"
#include "core/rendering/render_common.h"
#include "core/types.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace Seed {

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format)
    : Texture(type, w, h, format, SamplerProperty{}, nullptr) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 const u8 *image_data)
    : Texture(type, w, h, format, SamplerProperty{}, image_data) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 const SamplerProperty &property)
    : Texture(type, w, h, format, property, nullptr) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 const SamplerProperty &property, const u8 *image_data)
    : Texture(type, w, h, format, MSAAType::SAMPLE_COUNT_1, property,
              image_data) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 MSAAType msaa_type, const SamplerProperty &property,
                 const u8 *image_data)
    : type(type), w(w), h(h), format(format), property(property) {
    handle =
        RHI::alloc_texture(type, w, h, format, msaa_type, image_data, property);
    RHI::query_texture_size(handle, &real_w, &real_h);
}

void Texture::update(const u8 *data, u32 w, u32 h) {
    RHI::update(handle, format, 0, 0, 0, w, h, (void *)data);
}

void Texture::update_sampler(const SamplerProperty &property) {
    this->property = property;
    RHI::update_texture_sampler(handle, 0, property);
}
Texture::~Texture() { RHI::dealloc(handle); }

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

void MappableTexture::save_disk(const Path &path) {
    void *data = get_mapped();
    u32 pixel_size = get_pixel_format_size(format);
    if (w != real_w) {
        /* if the layout is different, we copy line by line */
        /* else we copy line by line */
        void *tmp = malloc(w * h * pixel_size);
        for (u32 i = 0; i < h; i++) {
            memcpy((void *)((u64)tmp + i * w * pixel_size),
                   (void *)((u64)data + i * real_w * pixel_size),
                   w * pixel_size);
        }
        stbi_write_png(path.data(), w, h, get_pixel_format_size(format), tmp,
                       0);
        free(tmp);
    } else {
        stbi_write_png(path.data(), w, h, get_pixel_format_size(format), data,
                       0);
    }
}


TextureArray::TextureArray(TextureType type, u32 w, u32 h, u32 layers,
                           PixelFormat format,
                           const SamplerProperty &property) {
    this->type = type;
    this->w = w;
    this->h = h;
    this->format = format;
    this->property = property;
    this->layers = layers;
    handle = RHI::alloc_textures(type, w, h, format, layers, property);
    RHI::query_texture_size(handle, &real_w, &real_h);
}
void TextureArray::update_layer(u32 w, u32 h, u32 layer, const void *data) {
    EXPECT_INDEX_INBOUND(layer, this->layers);
    RHI::update(handle, format, layer, 0, 0, w, h, data);
}

TextureCubemap::TextureCubemap(u32 w, u32 h, PixelFormat format,
                               const SamplerProperty &property) {
    this->type = type;
    this->w = w;
    this->h = h;
    this->format = format;
    this->property = property;
    handle = RHI::alloc_cubemap(w, h, format, property);
    RHI::query_texture_size(handle, &real_w, &real_h);
}

void TextureCubemap::update_face(u32 w, u32 h, CubemapFace face,
                                 const void *data) {
    EXPECT_INDEX_INBOUND((u32)face, 6);

    RHI::update(handle, format, (u32)face, 0, 0, w, h, data);
}

}  // namespace Seed