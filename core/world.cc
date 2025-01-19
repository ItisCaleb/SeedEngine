#include "world.h"

namespace Seed {
std::vector<Entity *> &World::get_entities() { return entities; }
std::vector<ModelEntity *> &World::get_model_entities() {
    return model_entities;
}
void World::add_entity(Entity *entity) { this->entities.push_back(entity); }

void World::add_model_entity(ModelEntity *entity) {
    this->model_entities.push_back(entity);
}

void World::tick(f32 dt) {
    for (Entity *e : this->entities) {
        e->update(dt);
    }
    RenderCommandDispatcher dp;
    for (Entity *e : this->entities) {
        e->render(dp);
    }
}

}  // namespace Seed