#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include "core/collision/shape.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_shape.h"
#include "core/ref.h"
#include "core/rendering/instance_data.h"
#include "core/resource/model.h"
#include "core/world/terrain.h"
#include "core/world/sky.h"
#include "core/rendering/light.h"
#include "core/resource/billboard.h"
#include "core/transform.h"
#include "core/world/entity.h"
#include "core/resource/world_setting.h"
#include <unordered_map>
#include <vector>

namespace Seed {
class WorldChunk : public RefCounted {
    private:
        Ref<Terrain> terrain;
        AABB aabb;
        std::vector<PointLight> point_lights;
        /* static objects, since they don't move at all, we won't store
         * tranform*/
        std::vector<PhysicBody> physic_bodies;
        std::vector<Ref<Model>> models;
        std::unordered_map<Model *, Ref<InstanceData>> model_instances;
        PhysicBody create_object_physic(const Transform &transform,
                                        const PhysicShape shape);
        void register_model_instance(Ref<Model> model,
                                     Ref<InstanceData> instance);

    public:
        WorldChunk(Ref<Terrain> terrain);
        void add_point_light(const PointLight &light) {
            this->point_lights.push_back(light);
        }

        void add_object(Transform &transform, const PhysicShape shape) {
            PhysicBody body = create_object_physic(transform, shape);
            physic_bodies.push_back(body);
            models.push_back({});
        }

        template <typename T, typename... Args>
        void add_object(Transform &transform, const PhysicShape shape,
                        Ref<T> model, Args &&...args) {
            PhysicBody body = create_object_physic(transform, shape);
            Ref<Model> _model = ref_cast<Model>(model);
            physic_bodies.push_back(body);
            models.push_back(_model);
            if (model.is_null()) return;
            auto iter = model_instances.find(*_model);
            if (iter == model_instances.end()) {
                Ref<InstanceData> instance = model->create_instance();
                register_model_instance(_model, instance);
                model->add_instance(instance, args...);
            } else {
                model->add_instance(iter->second, args...);
            }
        }

        Ref<Terrain> get_terrain() { return terrain; }
        ~WorldChunk();
};

class World {
    private:
        Ref<WorldSetting> setting;
        Ref<Sky> sky;
        Vec3 ambient_light;
        DirectionalLight direction_light;
        std::vector<PointLight> point_lights;
        Camera camera;
        std::vector<Ref<WorldChunk>> chunks;
        std::unordered_map<Model *, Ref<InstanceData>> model_instances;
        Ref<Terrain> terrain;

        EntityManager entity_manager;
        void register_engine_components();
        void register_model_instance(Ref<Model> model,
                                     Ref<InstanceData> instance);

    public:
        Ref<Sky> get_sky();
        Vec3 get_ambient_light() { return ambient_light; }
        DirectionalLight &get_direction_light() { return direction_light; }
        std::vector<PointLight> &get_point_lights() { return point_lights; }
        std::vector<Ref<WorldChunk>> &get_chunks() { return chunks; }
        Camera &get_camera() { return camera; }
        void tick(f32 dt);

        EntityManager &ecs() { return entity_manager; }

        World(/* args */);
        void load_setting(Ref<WorldSetting> setting);
        ~World() = default;
};

}  // namespace Seed

#endif