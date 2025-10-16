#include "entity.h"
#include "core/physic/physic_engine.h"
#include "core/rendering/mesh_storage.h"

namespace Seed {

void Entity::create_body(PhysicShape &shape, PhysicBodyType type) {
    PhysicEngine::get_instance()->create_body(this->body, shape, type,
                                              this->transform->get_position(),
                                              this->transform->get_rotation());
}
void Entity::remove_body() {
    PhysicEngine::get_instance()->delete_body(this->body);
}

void Entity::bind_model(Ref<Model> new_model) {
    if (!this->model.is_null() && !this->model->get_meshes().empty()) {
        this->model->remove_transform(this->transform);
    }
    this->model = new_model;
    if (new_model.is_null()) return;
    std::vector<Ref<Mesh>> &meshes = new_model->get_meshes();
    if (meshes.empty()) return;
    this->model->insert_transform(this->transform);
}

Entity::Entity(Vec3 position) {
    this->transform.create();
    this->transform->set_position(position);
}

Entity::Entity() : Entity(Vec3{0, 0, 0}) {}

Entity::~Entity() {
    if (!this->model.is_null() && !this->model->get_meshes().empty()) {
        this->model->remove_transform(this->transform);
    }
}

}  // namespace Seed