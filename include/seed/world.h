#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include <seed/entity.h>

#include <vector>

namespace Seed {
class World {
   private:
    std::vector<Entity *> entities;

   public:
    std::vector<Entity *>& get_entities();
    void add_entity(Entity *entity);
    World(/* args */) = default;
    ~World() = default;
};

}  // namespace Seed

#endif