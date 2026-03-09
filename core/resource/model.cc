#include "model.h"
#include "core/math/mat4.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {

// static AABB calculate_aabb(const std::vector<ModelVertex> &vertices) {
//     f32 x1 = 1e5, x2 = -1e5;
//     f32 y1 = 1e5, y2 = -1e5;
//     f32 z1 = 1e5, z2 = -1e5;
//     for (auto &vertex : vertices) {
//         x1 = std::min(x1, vertex.position.x);
//         x2 = std::max(x2, vertex.position.x);
//         y1 = std::min(y1, vertex.position.y);
//         y2 = std::max(y2, vertex.position.y);
//         z1 = std::min(z1, vertex.position.z);
//         z2 = std::max(z2, vertex.position.z);
//     }

//     f32 w = (x2 - x1) / 2;
//     f32 h = (y2 - y1) / 2;
//     f32 d = (z2 - z1) / 2;
//     return AABB{Vec3{x2 - w, y2 - h, z2 - d}, Vec3{w, h, d}};
// }

Model::Model(const std::vector<Ref<Mesh>> &meshes) : meshes(std::move(meshes)) {
    instances.create();
    for (Ref<Mesh> mesh : meshes) {
        MeshStorage::get_instance()->add_mesh(
            mesh, ref_cast<InstanceData>(instances));
    }
}

void Model::insert_transform(Ref<Transform> transform) {
    this->instances->insert_transform(transform);
}
void Model::remove_transform(Ref<Transform> transform) {
    this->instances->remove_transform(transform);
}

Model::~Model() {
    for (Ref<Mesh> mesh : meshes) {
        MeshStorage::get_instance()->remove_mesh(mesh);
    }
}

SkeletonModel::SkeletonModel(const std::vector<Ref<Mesh>> &meshes,
                             Ref<Skeleton> skeleton)
    : meshes(std::move(meshes)) {
    instances.create(skeleton);
    this->skeleton = skeleton;
    for (Ref<Mesh> mesh : meshes) {
        MeshStorage::get_instance()->add_mesh(
            mesh, ref_cast<InstanceData>(instances));
    }
}

void SkeletonModel::insert_instance(Ref<Transform> transform,
                                    Ref<AnimationState> state) {
    this->instances->insert_instance(transform, state);
}

SkeletonModel::~SkeletonModel() {
    for (Ref<Mesh> mesh : meshes) {
        MeshStorage::get_instance()->remove_mesh(mesh);
    }
}

}  // namespace Seed