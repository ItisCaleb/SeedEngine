#include "entity.h"
#include "core/physic/physic_engine.h"

namespace Seed {

Vec3 Entity::get_position() { return position; }
Vec3 Entity::get_rotation() { return rotation.to_euler(); }
Vec3 Entity::get_scale() { return scale; }

void Entity::set_position(Vec3 position) {
    this->position = position;
    this->dirty = true;
}
void Entity::set_rotation(Vec3 rotation) {
    this->rotation = Quaternion::from_euler(rotation);
    this->dirty = true;
}
void Entity::set_scale(Vec3 scale) {
    this->scale = scale;
    this->dirty = true;
}

void Entity::rotate(f32 x_angle, f32 y_angle, f32 z_angle) {
    this->rotation *= Quaternion::from_euler(x_angle, y_angle, z_angle);
    this->dirty = true;
}

void Entity::update_transform() {
    Mat4 transform;
    transform *= Mat4::translate_mat(position);
    transform *= Mat4::rotate_mat(rotation);
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

void Entity::create_body(PhysicShape &shape, PhysicBodyType type) {
    PhysicEngine::get_instance()->create_body(
        this->body, shape, type, this->position,
        this->rotation);
}
void Entity::remove_body() {
    PhysicEngine::get_instance()->delete_body(this->body);
}

Entity::Entity(Vec3 position)
    : position(position), rotation(Quaternion::identity()), scale(Vec3{1, 1, 1}) {}

Entity::Entity() : Entity(Vec3{0, 0, 0}) {}

}  // namespace Seed