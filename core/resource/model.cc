#include "model.h"
#include "core/math/mat4.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {

Model::Model(const std::vector<Ref<Mesh>> &meshes)
    : meshes(std::move(meshes)) {
        Ref<InstanceData> instance;
        instance.create();
        for(Ref<Mesh> mesh: meshes){
            MeshStorage::get_instance()->add_mesh(mesh, instance);
        }
    }

Model::~Model() {}

}  // namespace Seed