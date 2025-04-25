#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include "core/rendering/api/render_resource.h"
#include "core/ref.h"


namespace Seed
{
    class Texture : public RefCounted{
        RenderResource tex_rc;
        u32 w, h;

    public:
        RenderResource* get_render_resource(){
            return &tex_rc;
        }

        Texture(u32 w, u32 h, const char *image_data);
        ~Texture();
    };
} // namespace Seed


#endif