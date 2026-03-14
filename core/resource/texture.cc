#include "texture.h"
#include "core/rendering/rhi/render_command.h"

namespace Seed {
Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 const u8 *image_data)
    : Texture(type, w, h, format, image_data, SamplerProperty{}) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 const u8 *image_data, const SamplerProperty &property)
    : Texture(type, w, h, format, MSAAType::SAMPLE_COUNT_1, image_data,
              property) {}

Texture::Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                 MSAAType msaa_type, const u8 *image_data,
                 const SamplerProperty &property)
    : type(type), w(w), h(h), format(format), property(property) {
    handle =
        RHI::alloc_texture(type, w, h, format, msaa_type, image_data, property);
}

void Texture::update(const u8 *data, u32 w, u32 h) {
    RHI::update(handle, format, 0, 0, 0, w, h, (void *)data);
}

void Texture::upload_cube_map(const u8 *right, const u8 *left, const u8 *top,
                              const u8 *bottom, const u8 *front,
                              const u8 *back) {
    RHI::update(handle, format, 0, 0, 0, w, h, (void *)right);
    RHI::update(handle, format, 1, 0, 0, w, h, (void *)left);
    RHI::update(handle, format, 2, 0, 0, w, h, (void *)top);
    RHI::update(handle, format, 3, 0, 0, w, h, (void *)bottom);
    RHI::update(handle, format, 4, 0, 0, w, h, (void *)front);
    RHI::update(handle, format, 5, 0, 0, w, h, (void *)back);
}
Texture::~Texture() { RHI::dealloc(handle); }
}  // namespace Seed