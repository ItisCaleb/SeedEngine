#include "texture.h"

namespace Seed {
Texture::Texture(TextureType type, u32 w, u32 h, const u8 *image_data)
    : type(type), w(w), h(h) {
    tex_rc.alloc_texture(type, w, h, image_data);
}
Texture::~Texture() { tex_rc.dealloc(); }
}  // namespace Seed