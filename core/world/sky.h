#ifndef _SEED_SKY_H_
#define _SEED_SKY_H_
#include "core/ref.h"
#include "core/resource/material.h"
#include "core/resource/default_storage.h"

namespace Seed {
class SkyMaterial : public Material {
    public:
        SkyMaterial(Ref<Texture> cube_map)
            : Material(DS::get_instance()->sky_shader) {
            this->set_texture("skybox", cube_map);
        }
};

class Sky : public RefCounted {
    private:
        Ref<SkyMaterial> sky_mat;
        Ref<TextureCubemap> cubemap;

    public:
        Sky(Ref<TextureCubemap> cubemap) {
            sky_mat.create(ref_cast<Texture>(cubemap));
        }
        Ref<SkyMaterial> get_material() { return sky_mat; }
        ~Sky() {}
};

}  // namespace Seed

#endif