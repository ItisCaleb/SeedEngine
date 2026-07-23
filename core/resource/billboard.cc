#include "billboard.h"
#include "core/ref.h"
#include "core/resource/default_storage.h"
#include "core/system.h"

namespace Seed {
BillboardMaterial::BillboardMaterial(Ref<Texture> tex)
    : Material(System::gDefaultStorage->billboard_shader) {
    set_texture("u_billboard", tex);
    depth_state.depth_mode = DepthMode::ALPHA_TEST;
}

Billboard::Billboard(Ref<Texture> texture) {
    material.create(texture);
    billboard_mesh.create(System::gDefaultStorage->quad_vertices,
                          ref_cast<Material>(material),
                          AABB{Vec3{0.5, 0.5, 0}, Vec3{0.5, 0.5, 0}});
    // System::gRenderEngine->get_mesh_storage()->add_mesh(billboard_mesh,
    //                                       ref_cast<InstanceBatch>(instances));
}

Billboard::~Billboard() {}
}  // namespace Seed
