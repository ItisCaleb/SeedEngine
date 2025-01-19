#include "model_entity.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/material.h"

namespace Seed {

void ModelEntity::render(RenderCommandDispatcher &dp) {

}

void ModelEntity::set_material(Material mat) { this->material = mat; }

void ModelEntity::set_texture(RenderResource tex) { this->tex = tex; }

Ref<Mesh> ModelEntity::get_mesh() { return mesh; }

ModelEntity::ModelEntity(Vec3 position, Ref<Mesh> model)
    : Entity(position), mesh(model) {
    this->material_rc = &RenderResource::constants["Material"];
}
ModelEntity::ModelEntity(Ref<Mesh> model) : ModelEntity(Vec3{0, 0, 0}, model) {}

}  // namespace Seed
