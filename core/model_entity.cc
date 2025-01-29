#include "model_entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"
#include "core/rendering/model.h"

namespace Seed {

void ModelEntity::update(f32 dt) {
    Vec3 rot = this->get_rotation();
    rot.x += dt;
    rot.y += dt;
    rot.z += dt;
    this->set_rotation(rot);
}

void ModelEntity::render(RenderCommandDispatcher &dp) {}

void ModelEntity::set_material_variant(u32 variant) {
    this->mat_variant = variant;
}

Ref<Model> ModelEntity::get_model() { return model; }

u32 ModelEntity::get_material_variant() { return mat_variant; }

AABB ModelEntity::get_model_aabb() {
    if(model.is_null()) return {};
    AABB bounding_box = model->get_bounding_box();
    bounding_box.center += position;
    bounding_box.ext *= scale;
    return bounding_box;
}

ModelEntity::ModelEntity(Vec3 position, Ref<Model> model)
    : Entity(position), model(model) {}
ModelEntity::ModelEntity(Ref<Model> model)
    : ModelEntity(Vec3{0, 0, 0}, model) {}

}  // namespace Seed
