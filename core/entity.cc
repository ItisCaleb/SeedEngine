#include "entity.h"

namespace Seed {

Vec3 Entity::get_position() { return position; }
Vec3 Entity::get_rotation() { return rotation; }
Vec3 Entity::get_scale() { return scale; }

void Entity::set_position(Vec3 position) { this->position = position; }
void Entity::set_rotation(Vec3 rotation) { this->rotation = rotation; }
void Entity::set_scale(Vec3 scale) { this->scale = scale; }

Entity::Entity(Vec3 position)
    : position(position), rotation(Vec3{0, 0, 0}), scale(Vec3{1, 1, 1}) {}

Entity::Entity() : Entity(Vec3{0, 0, 0}) {}

}  // namespace Seed