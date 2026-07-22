#include "entity.h"
#include <cassert>
#include <cstdlib>

namespace Seed {

Entity EntityManager::create_entity() {
    if (free_entities.empty()) {
        entity_component_masks[next_entity] = 0;
        alive_entities.push_back(next_entity);
        return next_entity++;
    }
    u32 entity = free_entities.back();
    entity_component_masks[entity] = 0;
    alive_entities.push_back(entity);
    free_entities.pop_back();
    return entity;
}

void EntityManager::destroy_entity(Entity entity) {
    assert(entity > -1);
    /* TODO: handle component destruction */
    free_entities.push_back(entity);
    if (entity == alive_entities.back()) {
        alive_entities.pop_back();
    } else {
        alive_entities[entity] = alive_entities.back();
    }
}

void *EntityManager::_create_or_get_components(u64 component_id,
                                               u64 element_size) {
    auto iter = components.find(component_id);
    if (iter == components.end()) {
        void *component_array = malloc(ENTITY_MAX * element_size);
        components[component_id] = component_array;
        component_bit[component_id] = (1 << next_component_bit);
        next_component_bit++;
        return component_array;
    }
    return iter->second;
}

EntityManager::EntityManager() { entity_component_masks.resize(ENTITY_MAX); }
EntityManager::~EntityManager() {
    for (auto [_, component_array] : this->components) {
        free(component_array);
    }
}

}  // namespace Seed