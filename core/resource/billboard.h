#ifndef _SEED_BILLBOARD_H_
#define _SEED_BILLBOARD_H_
#include "core/ref.h"
#include "core/resource/material.h"
#include "core/rendering/mesh.h"

namespace Seed {
class BillboardMaterial : public Material {
    public:
        BillboardMaterial(Ref<Texture> tex);
};

class Billboard : public Resource {
    private:
        Ref<BillboardMaterial> material;
        Ref<Mesh> billboard_mesh;

    public:
        Billboard(Ref<Texture> texture);
        ~Billboard();
};

}  // namespace Seed

#endif
