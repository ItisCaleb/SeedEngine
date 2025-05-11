#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include "core/rendering/api/render_resource.h"
#include "core/resource/resource.h"


namespace Seed
{
    class Texture : public Resource{
    private:
        RenderResource tex_rc;
        u32 w, h;

    public:
        RenderResource* get_render_resource(){
            return &tex_rc;
        }
        u32 get_width(){
            return w;
        }

        u32 get_height(){
            return h;
        }

        Texture(u32 w, u32 h, const u8 *image_data);
        ~Texture();
    };
} // namespace Seed


#endif