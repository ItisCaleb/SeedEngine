#ifndef _SEED_MESH_STORAGE_H_
#define _SEED_MESH_STORAGE_H_
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include "core/resource/model.h"
#include "core/types.h"
#include <unordered_map>

namespace Seed {

class MeshStorage {
        struct MeshInstance {
                Ref<Mesh> mesh;
                Ref<InstanceData> instance;
        };

    private:
        std::unordered_map<u64, MeshInstance> meshes;

    public:
        void add_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance);
        void remove_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance);
        void add_model(Ref<Model> model, Ref<InstanceData> instance);
        void remove_model(Ref<Model> model, Ref<InstanceData> instance);
        std::unordered_map<u64, MeshInstance> &get_meshes() { return meshes; }
        MeshStorage();
        ~MeshStorage();
};
}  // namespace Seed

#endif
