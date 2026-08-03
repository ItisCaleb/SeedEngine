#include "world.h"

#include <fmt/base.h>
#include "behaviour.h"
#include "core/debug/profiler.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_engine.h"
#include "core/ref.h"
#include "core/rendering/instance_batch.h"
#include "core/rendering/mesh_storage.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/animation.h"
#include "core/resource/model.h"
#include "core/resource/texture.h"
#include "core/transform.h"
#include "core/world/components.h"
#include "core/resource/resource_loader.h"
#include "core/rendering/rhi/render_engine.h"
#include "entity.h"

namespace Seed {

WorldChunk::WorldChunk() {}

PhysicBody WorldChunk::create_object_physic(const Transform &transform,
                                            const PhysicShape shape) {
    PhysicBody body;
    System::gPhysicEngine->create_body(body, shape, PhysicBodyType::STATIC,
                                       transform.get_position(),
                                       transform.get_rotation());
    return body;
}

void WorldChunk::register_model_instance(Ref<Model> model,
                                         Ref<InstanceBatch> instance) {
    model_instances[*model] = instance;
    System::gRenderEngine->get_mesh_storage()->add_model(model, instance);
}

WorldChunk::~WorldChunk() {
    for (PhysicBody &body : physic_bodies) {
        System::gPhysicEngine->delete_body(body);
    }
    for (auto [model, instance] : model_instances) {
        System::gRenderEngine->get_mesh_storage()->remove_model(model,
                                                                instance);
    }
}

Ref<Sky> World::get_sky() { return sky; }

void World::tick(f32 dt) {
    PROFILE_SCOPE("World");

    PhysicEngine *phys = System::gPhysicEngine;

    // sync transform to physic
    entity_manager.run_system<Transform, PhysicBody>(
        [=](Entity e, Transform *tf, PhysicBody *ph) {
            if (ph->type != PhysicBodyType::STATIC) {
                phys->set_physics(*ph, tf->position, tf->rotation);
            }
        });

    System::gPhysicEngine->process();

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
            Ref<InstanceBatch> data = model_instances[*inst->model];
            inst->model->_add_instance(data, *tf);
        });
    entity_manager.run_system<Transform, SkeletonMeshInstance>(
        [&](Entity e, Transform *tf, SkeletonMeshInstance *inst) {
            Ref<InstanceBatch> data = model_instances[*inst->model];
            AnimationState *state =
                entity_manager.query_component<AnimationState>(e);
            if (state) {
                state->update(1 / 30.0f);
            }
            inst->model->_add_instance(data, *tf, state);
        });
}

void World::register_model_instance(Ref<Model> model,
                                    Ref<InstanceBatch> instance) {
    model_instances[*model] = instance;
    System::gRenderEngine->get_mesh_storage()->add_model(model, instance);
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

            System::gPhysicEngine->create_body(
                *ph, shape, type, tf->get_position(), tf->get_rotation());
        });
    ecs.on_add<MeshInstance>([&](EntityManager &m, Entity e, MeshInstance *ph) {
        if (ph->model.is_null()) return;
        Ref<Model> _model = ref_cast<Model>(ph->model);
        auto iter = model_instances.find(*_model);
        if (iter == model_instances.end()) {
            Ref<InstanceBatch> instance = ph->model->create_instance();
            register_model_instance(_model, instance);
        }
    });
    ecs.on_add<SkeletonMeshInstance>(
        [&](EntityManager &m, Entity e, SkeletonMeshInstance *ph) {
            if (ph->model.is_null()) return;
            Ref<Model> _model = ref_cast<Model>(ph->model);
            auto iter = model_instances.find(*_model);
            if (iter == model_instances.end()) {
                Ref<InstanceBatch> instance = ph->model->create_instance();
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

World::World() {
    this->setting.create();
    this->terrain.create();
    this->ambient_light = Vec3{0.25, 0.25, 0.25};
    this->camera.set_position(Vec3{0, 20, 0});
    this->camera.set_perspective(45, 1.33, 0.1, 2000.0);
    register_engine_components();
}

void World::load_setting(Ref<WorldSetting> setting) {
    if (setting.is_null()) return;

    this->setting = setting;
    if (System::gResourceLoader == nullptr) return;
    ResourceLoader *loader = System::gResourceLoader;

    point_lights.clear();
    terrain->reset_texture_palette();
    for (u32 layer = 0; layer < setting->terrain_textures.size() &&
                        layer < TERRAIN_TEXTURE_LAYERS;
         layer++) {
        if (setting->terrain_textures[layer].is_null()) continue;
        terrain->update_texture_layer(
            layer, loader->load_image_to_upload(
                       setting->terrain_textures[layer], true));
    }
    for (u32 layer = 0; layer < setting->terrain_normals.size() &&
                        layer < TERRAIN_TEXTURE_LAYERS;
         layer++) {
        if (setting->terrain_normals[layer].is_null()) continue;
        terrain->update_normal_layer(
            layer, loader->load_image_to_upload(setting->terrain_normals[layer],
                                                true));
    }

    terrain->clear_chunks();
    Ref<Image> default_heightmap = Terrain::create_default_heightmap();
    Ref<Image> default_controlmap = Terrain::create_default_controlmap();

    for (u32 chunk_index = 0; chunk_index < setting->chunks.size() &&
                              chunk_index < TERRAIN_CHUNK_LAYERS;
         chunk_index++) {
        const ChunkSetting &chunk = setting->chunks[chunk_index];
        Ref<Image> heightmap = loader->load_image(chunk.height_map);
        Ref<Image> controlmap = loader->load_image(chunk.control_map);
        if (heightmap.is_null()) {
            heightmap = default_heightmap;
        }
        if (controlmap.is_null()) {
            controlmap = default_controlmap;
        }
        terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
    }

    Ref<TextureCubemap> sky_cubemap = loader->load_cubemap(
        2048, 2048, setting->sky.right, setting->sky.left, setting->sky.up,
        setting->sky.down, setting->sky.front, setting->sky.back);
    if (sky_cubemap.is_null()) {
        sky = nullptr;
    } else {
        sky.create(sky_cubemap);
    }

    direction_light = DirectionalLight(setting->dir_light.direction,
                                       setting->dir_light.diffuse,
                                       setting->dir_light.specular, true);

    for (const ChunkSetting &chunk : setting->chunks) {
        for (const PointLightSetting &light : chunk.lights) {
            point_lights.emplace_back(light.position, light.diffuse,
                                      light.specular);
        }
    }
}

}  // namespace Seed
