#include <seed/world.h>

namespace Seed{
    std::vector<Entity *>& World::get_entities(){
        return entities;
    }
    void World::add_entity(Entity *entity){
        this->entities.push_back(entity);
    }

}