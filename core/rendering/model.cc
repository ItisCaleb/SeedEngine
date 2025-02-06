#include "model.h"
#include "core/math/mat4.h"
#include "api/render_resource.h"

namespace Seed {

Model::Model(const std::vector<Mesh> &meshes,
             const std::vector<Ref<Material>> &mats, AABB bounding_box)
    : meshes(std::move(meshes)), model_mat(mats), bounding_box(bounding_box) {
        instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    }


AABB Model::get_bounding_box() { return bounding_box; }

Model::~Model() {
    instance_rc.dealloc();
    for (Mesh &mesh : meshes) {
        mesh.vertices_rc.dealloc();
        mesh.indices_rc.dealloc();
    }
}


ModelMaterial::ModelMaterial(const std::vector<Ref<Material>> &mat) {
    this->materials.push_back(mat);
}
std::vector<Ref<Material>> *ModelMaterial::get_materials(u32 variant) {
    if (variant >= materials.size()) {
        return nullptr;
    }
    return &materials[variant];
}

Ref<Material> ModelMaterial::get_material(u32 material_handle, u32 variant) {
    std::vector<Ref<Material>> *mats = get_materials(variant);
    if (mats == nullptr || material_handle >= mats->size()) {
        return Ref<Material>();
    }
    return (*mats)[material_handle];
}
void ModelMaterial::add_variant(std::vector<Ref<Material>> &variant) {
    this->materials.push_back(variant);
}

}  // namespace Seed