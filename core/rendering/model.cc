#include "model.h"
#include "core/math/mat4.h"
#include "api/render_resource.h"

namespace Seed {

Model::Model(const std::vector<Mesh> &meshes, const std::vector<Ref<Material>> &mats)
    : meshes(std::move(meshes)), model_mat(mats) {}
Ref<Model> Model::create(const std::vector<Mesh> &meshes,
                         const std::vector<Ref<Material>> &mats) {
    Ref<Model> model(meshes, mats);
    std::vector<VertexAttribute> vertices_attrs = {
        {.layout_num = 0, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 1, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 2, .size = 2, .stride = sizeof(Vertex)}};
    std::vector<VertexAttribute> instance_attrs = {
        {
            .layout_num = 3,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 4,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 5,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 6,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
    };
    model->vertices_desc_rc.alloc_vertex_desc(vertices_attrs);
    model->instance_rc.alloc_vertex(sizeof(Mat4), 1, NULL);
    model->instance_desc_rc.alloc_vertex_desc(instance_attrs);
    return model;
}

Model::~Model() {
    vertices_desc_rc.dealloc();
    instance_rc.dealloc();
    instance_desc_rc.dealloc();
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