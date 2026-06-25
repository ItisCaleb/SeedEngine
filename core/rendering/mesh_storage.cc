#include "mesh_storage.h"
#include <vector>
#include "core/macro.h"
#include "core/misc//hash.h"
#include "instance_data.h"
#include "mesh.h"

namespace Seed {
MeshStorage::MeshStorage() { instance = this; }

void MeshStorage::add_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance) {
    EXPECT_NOT_NULL_RET(*mesh);
    EXPECT_NOT_NULL_RET(*instance);
    /* since mesh and instance address will be same across entire game */
    /* we can use it's address as hash key */
    Hash h;
    Mesh *mesh_ptr = mesh.ptr();
    InstanceData *instance_ptr = instance.ptr();
    h.update(&mesh_ptr);
    h.update(&instance_ptr);
    u64 key = h.digest();
    if (this->meshes.find(key) == this->meshes.end()) {
        this->meshes[key] = {.mesh = mesh, .instance = instance};
    } else {
        SEED_WARN("Mesh already in MeshStorage.");
    }
}

void MeshStorage::remove_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance) {
    EXPECT_NOT_NULL_RET(*mesh);
    EXPECT_NOT_NULL_RET(*instance);

    Hash h;
    Mesh *mesh_ptr = mesh.ptr();
    InstanceData *instance_ptr = instance.ptr();
    h.update(&mesh_ptr);
    h.update(&instance_ptr);
    u64 key = h.digest();
    this->meshes.erase(key);
}

void MeshStorage::add_model(Ref<Model> model, Ref<InstanceData> instance) {
    std::vector<Ref<Mesh>> meshes = model->get_meshes();
    for (Ref<Mesh> mesh : meshes) {
        add_mesh(mesh, instance);
    }
}
void MeshStorage::remove_model(Ref<Model> model, Ref<InstanceData> instance) {
    std::vector<Ref<Mesh>> meshes = model->get_meshes();
    for (Ref<Mesh> mesh : meshes) {
        remove_mesh(mesh, instance);
    }
}

MeshStorage::~MeshStorage() {}
}  // namespace Seed