#include "entity.h"

namespace Seed {

Vec3 Entity::get_position() { return position; }
Vec3 Entity::get_rotation() { return rotation; }
Vec3 Entity::get_scale() { return scale; }

void Entity::set_position(Vec3 position) {
    this->position = position;
    this->dirty = true;
}
void Entity::set_rotation(Vec3 rotation) {
    this->rotation = rotation;
    this->dirty = true;
}
void Entity::set_scale(Vec3 scale) {
    this->scale = scale;
    this->dirty = true;
}
void Entity::update_transform() {
    Mat4 transform;
    transform *= Mat4::translate_mat(position);
    transform *= Mat4::rotate_mat(rotation.y, Vec3{0, 1, 0});
    transform *= Mat4::rotate_mat(rotation.x, Vec3{1, 0, 0});
    transform *= Mat4::rotate_mat(rotation.z, Vec3{0, 0, 1});
    transform *= Mat4::scale_mat(scale);
    this->transform = transform;
}

Mat4 Entity::get_transform() {
    if (dirty) {
        update_transform();
        this->dirty = false;
    }
    return this->transform;
}

Entity::Entity(Vec3 position)
    : position(position), rotation(Vec3{0, 0, 0}), scale(Vec3{1, 1, 1}) {}

Entity::Entity() : Entity(Vec3{0, 0, 0}) {}

}  // namespace Seed