#include "model_entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {

void ModelEntity::render(RenderCommandDispatcher &dp) {
    dp.begin();
    dp.use(&this->mesh_rc);
    Mat4 model = Mat4::translate_mat(this->get_position());
    Vec3 rotation = this->get_rotation();
    model *= Mat4::rotate_mat(rotation.z, Vec3{0, 0, 1});
    model *= Mat4::rotate_mat(rotation.y, Vec3{0, 1, 0});
    model *= Mat4::rotate_mat(rotation.x, Vec3{1, 0, 0});
    model *= Mat4::scale_mat(this->get_scale());
    Mat4 *matrices =
        (Mat4 *)dp.update(matrices_rc, sizeof(Mat4) * 2, sizeof(Mat4));
    matrices[0] = model.transpose();
    Material *mat = dp.update_all<Material>(material_rc);
    *mat = this->material;
    dp.use(&this->tex);
    dp.end();
}

void ModelEntity::set_material(Material mat) { this->material = mat; }

void ModelEntity::set_texture(RenderResource tex) { this->tex = tex; }

ModelEntity::ModelEntity(Vec3 position, Ref<Mesh> model) : Entity(position) {
    std::vector<u32> lens = {3, 3, 2};
    this->mesh_rc.alloc_vertex(sizeof(Vertex), lens, model->vertices.size(),
                               model->vertices.data());
    this->matrices_rc = &RenderResource::constants["Matrices"];
    this->material_rc = &RenderResource::constants["Material"];
}
ModelEntity::ModelEntity(Ref<Mesh> model) : ModelEntity(Vec3{0, 0, 0}, model) {}

}  // namespace Seed
