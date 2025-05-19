#include "model_entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/resource/material.h"
#include "core/resource/model.h"

namespace Seed {

void ModelEntity::update(f32 dt) {
    f32 speed = 90;
    //this->rotate(0, speed * dt, 0);
}

void ModelEntity::render() {}

Ref<Model> ModelEntity::get_model() { return model; }

AABB ModelEntity::get_model_aabb() {
    AABB result = {};
    if (model.is_null()) return result;
    AABB bounding_box = model->get_bounding_box();
    Mat4 rot_mat = Mat4::rotate_mat(Quaternion::from_euler(rotation));
    result.center = position;
    result.ext = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.center[i] +=
                rot_mat[i][j] * bounding_box.center[j] * scale[j];
            result.ext[i] +=
                abs(rot_mat[i][j]) * bounding_box.ext[j] * scale[j];
        }
    }
    return result;
}

ModelEntity::ModelEntity(Vec3 position, Ref<Model> model)
    : Entity(position), model(model) {}
ModelEntity::ModelEntity(Ref<Model> model)
    : ModelEntity(Vec3{0, 0, 0}, model) {}

}  // namespace Seed
