#ifndef _SEED_SKY_H_
#define _SEED_SKY_H_
#include "core/resource/material.h"
#include "core/resource/default_storage.h"

namespace Seed {
class SkyMaterial : public Material {
    public:
        SkyMaterial(Ref<Texture> cube_map)
            : Material(DS::get_instance()->get_sky_shader()) {
            this->add_texture_unit(cube_map);
        }
};

class Sky : public Resource {
    private:
        Ref<SkyMaterial> sky_mat;

    public:
        Sky(u32 w, u32 h, const u8 *right, const u8 *left, const u8 *top,
            const u8 *bottom, const u8 *front, const u8 *back) {
            Ref<Texture> cubemap;
            cubemap.create(TextureType::TEXTURE_CUBEMAP, w, h, nullptr);
            cubemap->upload_cube_map(right, left, top, bottom, front, back);
            sky_mat.create(cubemap);
        }
        Ref<SkyMaterial> get_material() { return sky_mat; }
        ~Sky() {}
};

}  // namespace Seed

#endif