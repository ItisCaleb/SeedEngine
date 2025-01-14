#include <seed/entity.h>

namespace Seed {
Ref<Mesh> Entity::get_mesh() { return model; }

void Entity::bind_mesh(Ref<Mesh> model) { this->model = model; }

Entity::Entity(Vec3 position, Vec3 rotation, Vec3 scale, Ref<Mesh> model)
    : position(position), rotation(rotation), scale(scale), model(model) {}

Entity::Entity(Vec3 position, Ref<Mesh> model)
    : Entity(position, Vec3{0, 0, 0}, Vec3{1, 1, 1}, model) {}

}  // namespace Seed