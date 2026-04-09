#include "texture.h"
#include "core/rendering/render_common.h"
#include "core/types.h"
#include "image.h"

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

TextureArray::TextureArray(TextureType type, u32 w, u32 h, u32 layers,
                           PixelFormat format,
                           const SamplerProperty &property) {
    this->type = type;
    this->w = w;
    this->h = h;
    this->format = format;
    this->property = property;
    handle = RHI::alloc_textures(type, w, h, format, layers, property);
    RHI::query_texture_size(handle, &real_w, &real_h);
}
void TextureArray::update_layer(u32 w, u32 h, u32 layer, const void *data) {
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
    RHI::update(handle, format, (u32)face, 0, 0, w, h, data);
}

}  // namespace Seed