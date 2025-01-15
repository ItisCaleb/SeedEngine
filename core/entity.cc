#include "entity.h"

namespace Seed {
Ref<Mesh> Entity::get_mesh() { return model; }

void Entity::bind_mesh(Ref<Mesh> model) { this->model = model; }

Vec3 Entity::get_position() { return position; }
Vec3 Entity::get_rotation() { return rotation; }
Vec3 Entity::get_scale() { return scale; }

void Entity::set_position(Vec3 position) { this->position = position; }
void Entity::set_rotation(Vec3 rotation) { this->rotation = rotation; }
void Entity::set_scale(Vec3 scale) { this->scale = scale; }

Entity::Entity(Vec3 position, Vec3 rotation, Vec3 scale, Ref<Mesh> model)
    : position(position), rotation(rotation), scale(scale), model(model) {}

Entity::Entity(Vec3 position, Ref<Mesh> model)
    : Entity(position, Vec3{0, 0, 0}, Vec3{1, 1, 1}, model) {}

Entity::Entity() : scale(Vec3{1, 1, 1}) {}

}  // namespace Seed