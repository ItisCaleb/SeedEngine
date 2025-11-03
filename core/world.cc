#include "world.h"

namespace Seed {
std::vector<Entity *> &World::get_entities() { return entities; }

Ref<Terrain> World::get_terrain() { return terrain; }
Ref<Sky> World::get_sky() { return sky; }

void World::add_entity(Entity *entity) { this->entities.push_back(entity); }

void World::set_terrain(Ref<Terrain> terrain) { this->terrain = terrain; }

void World::set_sky(Ref<Sky> sky) { this->sky = sky; }

void World::tick(f32 dt) {
    for (Entity *e : this->entities) {
        e->update(dt);
    }
}

World::World()
    : direction_light(Vec3{-0.5, -0.5, 0}, Vec3{0.8, 0.8, 0.8},
                      Vec3{0.4, 0.4, 0.4}) {
    this->ambient_light = Vec3{0.25, 0.25, 0.25};
}

}  // namespace Seed