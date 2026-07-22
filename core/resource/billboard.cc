#include "billboard.h"
#include "core/ref.h"

namespace Seed {
Billboard::Billboard(Ref<Texture> texture) {
    material.create(texture);
    billboard_mesh.create(System::gDefaultStorage->quad_vertices,
                          ref_cast<Material>(material),
                          AABB{Vec3{0.5, 0.5, 0}, Vec3{0.5, 0.5, 0}});
    // System::gRenderEngine->get_mesh_storage()->add_mesh(billboard_mesh,
    //                                       ref_cast<InstanceData>(instances));
}

Billboard::~Billboard() {}
}  // namespace Seed