#ifndef _SEED_SKY_H_
#define _SEED_SKY_H_
#include "core/resource/material.h"

namespace Seed {
class Sky : public Resource {
    private:
        Ref<Material> sky_mat;

    public:
        Sky(u32 w, u32 h, const u8 *right, const u8 *left, const u8 *top,
            const u8 *bottom, const u8 *front, const u8 *back) {
            Ref<Texture> cubemap;
            cubemap.create(TextureType::TEXTURE_CUBEMAP, w, h, nullptr);
            cubemap->upload_cube_map(right, left, top, bottom, front, back);
            sky_mat.create();
            sky_mat->set_texture_map(Material::TextureMapType::DIFFUSE, cubemap);
        }
        Ref<Material> get_material(){
            return sky_mat;
        }
        ~Sky() {}
};

}  // namespace Seed

#endif