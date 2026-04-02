#include "billboard.h"
#include "core/ref.h"
#include "core/rendering/instance_data.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {
Billboard::Billboard(Ref<Texture> texture) {
    material.create(texture);
    billboard_mesh.create(DS::get_instance()->quad_vertices,
                          ref_cast<Material>(material),
                          AABB{Vec3{0.5, 0.5, 0}, Vec3{0.5, 0.5, 0}});
    // MeshStorage::get_instance()->add_mesh(billboard_mesh,
    //                                       ref_cast<InstanceData>(instances));
}

Billboard::~Billboard() {

}
}  // namespace Seed