#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include <seed/entity.h>

#include <vector>

namespace Seed {
class World {
   private:
    std::vector<Entity *> entities;

   public:
    World(/* args */);
    ~World();
};

World::World(/* args */) {}

World::~World() {}

}  // namespace Seed

#endif