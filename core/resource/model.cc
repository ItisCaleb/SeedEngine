#include "model.h"
#include "core/math/mat4.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {

Model::Model(const std::vector<Ref<Mesh>> &meshes) : meshes(std::move(meshes)) {
    instances.create();
    for (Ref<Mesh> mesh : meshes) {
        MeshStorage::get_instance()->add_mesh(mesh, ref_cast<InstanceData>(instances));
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

}  // namespace Seed