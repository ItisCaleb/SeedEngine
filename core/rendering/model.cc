#include "model.h"
#include "core/math/mat4.h"
#include "api/render_resource.h"

namespace Seed {

Model::Model(const std::vector<Mesh> &meshes,
             const std::vector<Ref<Material>> &mats, AABB bounding_box)
    : meshes(std::move(meshes)), materials(std::move(mats)), bounding_box(bounding_box) {
        instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    }

Ref<Material> Model::get_material(u32 material_handle) {
    if (material_handle >= materials.size()) {
        return Ref<Material>();
    }
    return materials[material_handle];
}

AABB Model::get_bounding_box() { return bounding_box; }

Model::~Model() {
    instance_rc.dealloc();
    for (Mesh &mesh : meshes) {
        mesh.vertices_rc.dealloc();
        mesh.indices_rc.dealloc();
    }
}



}  // namespace Seed