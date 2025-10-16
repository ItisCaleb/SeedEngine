#include "mesh_storage.h"
#include "core/macro.h"

namespace Seed {
MeshStorage::MeshStorage() { instance = this; }

void MeshStorage::add_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance) {
    EXPECT_NOT_NULL_RET(*mesh);
    if (this->meshes.find(mesh) == this->meshes.end()) {
        this->meshes[mesh] = instance;
    } else {
        spdlog::error("Mesh already in MeshStorage.");
    }
}

void MeshStorage::remove_mesh(Ref<Mesh> mesh) {
    EXPECT_NOT_NULL_RET(*mesh);
    this->meshes.erase(mesh);
}

Ref<InstanceData> MeshStorage::get_mesh_instance(Ref<Mesh> mesh) {
    if (mesh.is_null()) return Ref<InstanceData>();
    if (this->meshes.find(mesh) == this->meshes.end()) {
        SPDLOG_WARN("Can't find instance with supplied mesh.");
        return Ref<InstanceData>();
    } else {
        return this->meshes[mesh];
    }
}

MeshStorage::~MeshStorage() {}
}  // namespace Seed