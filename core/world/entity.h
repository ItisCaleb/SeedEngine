#ifndef _SEED_ENTITY_H_
#define _SEED_ENTITY_H_
#include <spdlog/spdlog.h>
#include <array>
#include <cassert>
#include <functional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "components.h"
#include "core/types.h"
#include "core/misc/hash.h"
#include "core/misc/type_name.h"
namespace Seed {
const constexpr i32 ENTITY_MAX = 16384;
typedef i32 Entity;

class EntityManager {
    private:
        /* component */
        std::unordered_map<u64, void *> components;
        std::unordered_map<u64, u64> component_bit;
        std::unordered_map<
            u64, std::function<void(EntityManager &, Entity, void *, void *)>>
            on_add_hooks;

        void *_create_or_get_components(u64 component_id, u64 element_size);

        template <typename T>
        T *create_or_get_components(u64 component_id) {
            return (T *)_create_or_get_components(component_id, sizeof(T));
        }

        template <typename T>
        inline constexpr u64 get_type_id() {
            auto tname = type_name<T>();
            u64 component_id = fnv1a(tname);
            return component_id;
        }
        std::vector<u64> entity_component_masks;
        u32 next_component_bit = 0;

        /* entity */
        u32 next_entity = 0;
        std::vector<u32> alive_entities;
        std::vector<u32> free_entities;
        /* system */

        template <typename... Args, typename Fn, size_t... I>
        void _run_system(std::array<u64, sizeof...(Args)> &ids, u64 system_mask,
                         Fn &&system, std::index_sequence<I...>) {
            std::array<void *, sizeof...(Args)> args = {
                (void *)create_or_get_components<Args>(ids[I])...};
            for (u32 alive : alive_entities) {
                u32 entity_mask = entity_component_masks[alive];
                if ((system_mask & entity_mask) != system_mask) {
                    continue;
                }
                system(&static_cast<Args *>(args[I])[alive]...);
            }
        }

    public:
        Entity create_entity();
        void destroy_entity(Entity entity);

        template <typename T>
        bool has_component(Entity entity) {
            u64 component_id = get_type_id<T>();
            u64 result =
                entity_component_masks[entity] & component_bit[component_id];
            return result;
        }

        template <typename T>
        T *query_component(Entity entity) {
            u64 component_id = get_type_id<T>();

            T *component_array = create_or_get_components<T>(component_id);
            if (has_component<T>(entity)) {
                return &component_array[entity];
            }
            return nullptr;
        }

        template <typename T, typename... Args>
        void add_component(Entity entity, Args &&...args) {
            assert(entity > -1);
            u64 component_id = get_type_id<T>();
            T *component_array = create_or_get_components<T>(component_id);
            auto hook = on_add_hooks[component_id];

            if constexpr (has_create_args_v<T>) {
                new (&component_array[entity]) T;
                if (hook) {
                    typename ComponentTrait<T>::create_args tuple(
                        std::forward<Args>(args)...);
                    hook(*this, entity, &component_array[entity], &tuple);
                }
            } else {
                new (&component_array[entity]) T(args...);
                if (hook) {
                    std::tuple<> tuple;
                    hook(*this, entity, &component_array[entity], &tuple);
                }
            }
            entity_component_masks[entity] |= component_bit[component_id];
        }

        template <typename T, typename Fn>
        void on_add(Fn &&hook) {
            if constexpr (has_create_args_v<T>) {
                using args_t = typename ComponentTrait<T>::create_args;
                static_assert(std::is_invocable_v<Fn, EntityManager &, Entity,
                                                  T *, args_t>,
                              "on_add hook signature mismatch, expected: "
                              "Fn(EntityManager &, Entity, T *, create_args)");
            } else {
                static_assert(
                    std::is_invocable_v<Fn, EntityManager &, Entity, T *>,
                    "on_add hook signature mismatch, expected: "
                    "Fn(EntityManager &, Entity, T *)");
            }

            on_add_hooks[get_type_id<T>()] = [hook](EntityManager &w, Entity e,
                                                    void *component,
                                                    void *args) {
                T *comp = static_cast<T *>(component);
                if constexpr (has_create_args_v<T>) {
                    typename ComponentTrait<T>::create_args &tuple =
                        *static_cast<typename ComponentTrait<T>::create_args *>(
                            args);
                    hook(w, e, comp, tuple);
                } else {
                    hook(w, e, comp);
                }
            };
        }

        template <typename T>
        void remove_component(Entity entity) {
            assert(entity > -1);
            u64 component_id = get_type_id<T>();
            T *component_array = create_or_get_components<T>(component_id);
            new (&component_array[entity]) T;
            component_array[entity].~T();
            entity_component_masks[entity] &= ~component_bit[component_id];
        }

        template <typename... Args, typename Fn>
        void run_system(Fn &&system) {
            std::array<u64, sizeof...(Args)> ids{get_type_id<Args>()...};
            u64 system_mask = 0;
            for (u32 i = 0; i < sizeof...(Args); i++) {
                auto iter = component_bit.find(ids[i]);
                if (iter == component_bit.end()) {
                    return;
                }
                system_mask |= iter->second;
            }

            _run_system<Args...>(ids, system_mask, std::forward<Fn>(system),
                                 std::make_index_sequence<sizeof...(Args)>{});
        }
        EntityManager();
        ~EntityManager();
};
};  // namespace Seed

#endif