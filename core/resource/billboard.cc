#include "billboard.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {
Billboard::Billboard(Ref<Texture> texture) {
    material.create(texture);
    instances.create();
    billboard_mesh.create(DS::get_instance()->quad_vertices,
                          ref_cast<Material>(material),
                          AABB{Vec3{0.5, 0.5, 0}, Vec3{0.5, 0.5, 0}});
    MeshStorage::get_instance()->add_mesh(billboard_mesh,
                                          ref_cast<InstanceData>(instances));
}
void Billboard::insert_transform(Ref<Transform> transform) {
    this->instances->insert_transform(transform);
}
void Billboard::remove_transform(Ref<Transform> transform) {
    this->instances->remove_transform(transform);
}
Billboard::~Billboard() {
    MeshStorage::get_instance()->remove_mesh(billboard_mesh);
}
}  // namespace Seed