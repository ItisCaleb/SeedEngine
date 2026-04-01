#include "world.h"
#include <fmt/base.h>
#include <functional>
#include <string>
#include "behaviour.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_engine.h"
#include "core/ref.h"
#include "core/rendering/instance_data.h"
#include "core/rendering/mesh_storage.h"
#include "core/resource/animation.h"
#include "core/resource/model.h"
#include "core/resource/terrain.h"
#include "core/transform.h"
#include "core/world/components.h"
#include "entity.h"

namespace Seed {
WorldChunk::WorldChunk(Ref<Terrain> terrain) : terrain(terrain) {}

PhysicBody WorldChunk::create_object_physic(const Transform &transform,
                                            const PhysicShape shape) {
    PhysicBody body;
    PhysicEngine::get_instance()->create_body(
        body, shape, PhysicBodyType::STATIC, transform.get_position(),
        transform.get_rotation());
    return body;
}

void WorldChunk::register_model_instance(Ref<Model> model,
                                         Ref<InstanceData> instance) {
    model_instances[*model] = instance;
    MeshStorage::get_instance()->add_model(model, instance);
}

WorldChunk::~WorldChunk() {
    for (PhysicBody &body : physic_bodies) {
        PhysicEngine::get_instance()->delete_body(body);
    }
    for (auto [model, instance] : model_instances) {
        MeshStorage::get_instance()->remove_model(model, instance);
    }
}

Ref<Sky> World::get_sky() { return sky; }

void World::set_sky(Ref<Sky> sky) { this->sky = sky; }

void World::add_billboard(Ref<Billboard> billboard) {
    this->billboards.push_back(billboard);
}

void World::tick(f32 dt) {
    PhysicEngine *phys = PhysicEngine::get_instance();

    // sync transform to physic
    entity_manager.run_system<Transform, PhysicBody>(
        [=](Entity e, Transform *tf, PhysicBody *ph) {
            if (ph->type != PhysicBodyType::STATIC) {
                phys->set_physics(*ph, tf->position, tf->rotation);
            }
        });

    PhysicEngine::get_instance()->process();

    /* sync physic to transform */
    entity_manager.run_system<Transform, PhysicBody>(
        [=](Entity e, Transform *tf, PhysicBody *ph) {
            if (ph->type != PhysicBodyType::STATIC) {
                phys->query_physics(*ph, tf->position, tf->rotation);
                tf->dirty = true;
            }
        });
    /* run game logic update */
    entity_manager.run_system<BehaviourComponent>(
        [=](Entity e, BehaviourComponent *b) { b->behaviour->update(dt); });

    /* we collect entity tranform every frame */
    for (auto &[_, inst] : this->model_instances) {
        inst->clear();
    }
    /* upload model transform to instance*/
    entity_manager.run_system<Transform, MeshInstance>(
        [=](Entity e, Transform *tf, MeshInstance *inst) {
            Ref<InstanceData> data = model_instances[*inst->model];
            inst->model->_add_instance(data, *tf);
        });
    entity_manager.run_system<Transform, SkeletonMeshInstance>(
        [&](Entity e, Transform *tf, SkeletonMeshInstance *inst) {
            Ref<InstanceData> data = model_instances[*inst->model];
            AnimationState *state =
                entity_manager.query_component<AnimationState>(e);
            if (state) {
                state->update(1 / 30.0f);
            }
            inst->model->_add_instance(data, *tf, state);
        });
}

void World::add_chunk(Ref<WorldChunk> &chunk) {
    this->chunks.push_back(chunk);
    Ref<Terrain> terrain = chunk->get_terrain();
    if (terrains.count(terrain) == 0) {
        MeshStorage::get_instance()->add_mesh(
            terrain->get_mesh(),
            ref_cast<InstanceData>(terrain->get_instance()));
    }
}

void World::register_model_instance(Ref<Model> model,
                                    Ref<InstanceData> instance) {
    model_instances[*model] = instance;
    MeshStorage::get_instance()->add_model(model, instance);
}

void World::register_engine_components() {
    auto &ecs = this->entity_manager;
    ecs.on_add<PhysicBody>(
        [](EntityManager &m, Entity e, PhysicBody *ph,
           typename ComponentTrait<PhysicBody>::create_args args) {
            fmt::println("Create physic body!");
            auto &shape = std::get<0>(args);
            auto type = std::get<1>(args);
            Transform *tf = m.query_component<Transform>(e);
            if (!tf) {
                fmt::println(
                    "This entity doesn't have transform, create failed!");
                return;
            }

            PhysicEngine::get_instance()->create_body(
                *ph, shape, type, tf->get_position(), tf->get_rotation());
        });
    ecs.on_add<MeshInstance>([&](EntityManager &m, Entity e, MeshInstance *ph) {
        if (ph->model.is_null()) return;
        Ref<Model> _model = ref_cast<Model>(ph->model);
        auto iter = model_instances.find(*_model);
        if (iter == model_instances.end()) {
            Ref<InstanceData> instance = ph->model->create_instance();
            register_model_instance(_model, instance);
        }
    });
    ecs.on_add<SkeletonMeshInstance>(
        [&](EntityManager &m, Entity e, SkeletonMeshInstance *ph) {
            if (ph->model.is_null()) return;
            Ref<Model> _model = ref_cast<Model>(ph->model);
            auto iter = model_instances.find(*_model);
            if (iter == model_instances.end()) {
                Ref<InstanceData> instance = ph->model->create_instance();
                register_model_instance(_model, instance);
            }
        });
    ecs.on_add<BehaviourComponent>(
        [&](EntityManager &m, Entity e, BehaviourComponent *b) {
            if (b->behaviour.is_null()) return;
            b->behaviour->m = &m;
            b->behaviour->self = e;
            b->behaviour->start();
        });
}

World::World()
    : direction_light(Vec3{-0.5, -0.5, 0}, Vec3{0.8, 0.8, 0.8},
                      Vec3{0.4, 0.4, 0.4}) {
    this->ambient_light = Vec3{0.25, 0.25, 0.25};
    this->camera.set_position(Vec3{0, 20, 0});
    this->camera.set_perspective(45, 1.33, 0.1, 2000.0);
    register_engine_components();
}

}  // namespace Seed