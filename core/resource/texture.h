#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"

namespace Seed {

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
        Texture(){}
    public:
        TextureHandle get_handle() { return handle; }
        u32 get_width() { return w; }

        u32 get_height() { return h; }

        void update(const u8 *data, u32 w, u32 h);
        void upload_cube_map(const u8 *right, const u8 *left, const u8 *top,
                             const u8 *bottom, const u8 *front, const u8 *back);

        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                const u8 *image_data);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                const u8 *image_data, const SamplerProperty &property);
        Texture(TextureType type, u32 w, u32 h, PixelFormat format,
                MSAAType msaa_type, const u8 *image_data,
                const SamplerProperty &property);

        ~Texture();
};
}  // namespace Seed

#endif