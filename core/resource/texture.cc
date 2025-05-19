#include "texture.h"
#include "core/rendering/api/render_command.h"

namespace Seed {
Texture::Texture(TextureType type, u32 w, u32 h, const u8 *image_data)
    : type(type), w(w), h(h) {
    tex_rc.alloc_texture(type, w, h, image_data);
}

void Texture::upload_cube_map(const u8 *right, const u8 *left, const u8 *top,
                              const u8 *bottom, const u8 *front,
                              const u8 *back) {
    RenderCommandDispatcher dp(0);
    DEBUG_DISPATCH(dp);
    dp.update_cubemap(tex_rc, 0, 0, 0, w, h, (void *)right);
    dp.update_cubemap(tex_rc, 1, 0, 0, w, h, (void *)left);
    dp.update_cubemap(tex_rc, 2, 0, 0, w, h, (void *)top);
    dp.update_cubemap(tex_rc, 3, 0, 0, w, h, (void *)bottom);
    dp.update_cubemap(tex_rc, 4, 0, 0, w, h, (void *)front);
    dp.update_cubemap(tex_rc, 5, 0, 0, w, h, (void *)back);
}
Texture::~Texture() { tex_rc.dealloc(); }
}  // namespace Seed