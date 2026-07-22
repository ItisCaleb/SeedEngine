#ifndef _SEED_DECAL_H_
#define _SEED_DECAL_H_
#include "core/resource/resource.h"
#include "core/resource/material.h"
#include "core/resource/default_storage.h"
#include "core/system.h"

namespace Seed {
class DecalMaterial : public Material {
    public:
        DecalMaterial(Ref<Texture> texture)
            : Material(System::gDefaultStorage->decal_shader) {
            this->depth_state.depth_mode = DepthMode::ALPHA_TEST;
        }
};

class Decal : public Resource {
    private:
        Ref<DecalMaterial> mat;

    public:
        Ref<Material> get_material() { return ref_cast<Material>(mat); }
        Decal();
};
}  // namespace Seed

#endif
