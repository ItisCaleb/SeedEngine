#include "model_entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {

void ModelEntity::update(f32 dt) {
    Vec3 rot = this->get_rotation();
    rot.x += dt;
    rot.y += dt;
    rot.z += dt;
    this->set_rotation(rot);
}

void ModelEntity::render(RenderCommandDispatcher &dp) {}

void ModelEntity::set_material(Ref<Material> mat) { this->mat = mat; }


Ref<Mesh> ModelEntity::get_mesh() { return mesh; }

Ref<Material> ModelEntity::get_material() { return mat; }


ModelEntity::ModelEntity(Vec3 position, Ref<Mesh> model)
    : Entity(position), mesh(model) {}
ModelEntity::ModelEntity(Ref<Mesh> model) : ModelEntity(Vec3{0, 0, 0}, model) {}

}  // namespace Seed
