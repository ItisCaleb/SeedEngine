#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include "entity.h"
#include "model_entity.h"

#include <vector>

namespace Seed {
class World {
   private:
    std::vector<Entity *> entities;
    std::vector<ModelEntity *> model_entities;

   public:
    std::vector<Entity *> &get_entities();
    std::vector<ModelEntity *> &get_model_entities();

    void tick(f32 dt);
    void add_entity(Entity *entity);
    void add_model_entity(ModelEntity *entity);


    template <typename T, typename... Args>
    void add_entity(const Args &...args) {
        add_entity(new T(args...));
    }
    World(/* args */) = default;
    ~World() = default;
};

}  // namespace Seed

#endif