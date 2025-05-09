#include "texture.h"

namespace Seed {
Texture::Texture(u32 w, u32 h, const char *image_data) : w(w), h(h) {
    tex_rc.alloc_texture(w, h, image_data);
}
Texture::~Texture() { tex_rc.dealloc(); }
}  // namespace Seed