#ifndef _SEED_MESH_STORAGE_H_
#define _SEED_MESH_STORAGE_H_
#include "core/rendering/mesh.h"
#include "core/rendering/instance_data.h"
#include <map>

namespace Seed {
class MeshStorage {
    private:
        inline static MeshStorage *instance = nullptr;
        std::map<Ref<Mesh>, Ref<InstanceData>> meshes;

    public:
        void add_mesh(Ref<Mesh> mesh, Ref<InstanceData> instance);
        void remove_mesh(Ref<Mesh> mesh);
        Ref<InstanceData> get_mesh_instance(Ref<Mesh> mesh);
        std::map<Ref<Mesh>, Ref<InstanceData>> &get_meshes() { return meshes; }
        MeshStorage();
        ~MeshStorage();
        static MeshStorage *get_instance() { return instance; }
};
}  // namespace Seed

#endif