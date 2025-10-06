#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include "entity.h"
#include "model_entity.h"
#include "core/resource/terrain.h"
#include "core/resource/sky.h"
#include "core/rendering/light.h"

#include <vector>

namespace Seed {
class World {
    private:
        std::vector<Entity *> entities;
        std::vector<ModelEntity *> model_entities;
        Ref<Terrain> terrain;
        Ref<Sky> sky;
        Vec3 ambient_light;
        Light direction_light;
        std::vector<Light> point_lights;

    public:
        std::vector<Entity *> &get_entities();
        std::vector<ModelEntity *> &get_model_entities();
        Ref<Terrain> get_terrain();
        Ref<Sky> get_sky();
        Vec3 get_ambient_light() { return ambient_light; }
        Light &get_direction_light() { return direction_light; }
        std::vector<Light> &get_point_lights() { return point_lights; }

        void tick(f32 dt);
        void add_entity(Entity *entity);
        void add_model_entity(ModelEntity *entity);
        void set_terrain(Ref<Terrain> terrain);
        void set_sky(Ref<Sky> sky);

        template <typename T, typename... Args>
        void add_entity(const Args &...args) {
            add_entity(new T(args...));
        }
        World(/* args */);
        ~World() = default;
};

}  // namespace Seed

#endif