#ifndef _SEED_BILLBOARD_H_
#define _SEED_BILLBOARD_H_
#include "core/ref.h"
#include "core/rendering/instance_data.h"
#include "core/resource/material.h"
#include "core/rendering/mesh.h"

namespace Seed {
class BillboardMaterial : public Material {
    public:
        BillboardMaterial(Ref<Texture> tex)
            : Material(DS::get_instance()->billboard_shader) {
            this->set_texture("u_billboard", tex);
            this->depth_state.depth_mode = DepthMode::ALPHA_TEST;
        }
};

class Billboard : public Resource {
    private:
        Ref<BillboardMaterial> material;
        Ref<TransformInstanceData> instances;
        Ref<Mesh> billboard_mesh;

    public:
        Billboard(Ref<Texture> texture);
        void insert_transform(Ref<Transform> transform);
        void remove_transform(Ref<Transform> transform);
        ~Billboard();
};

}  // namespace Seed

#endif