#ifndef _SEED_WORLD_H_
#define _SEED_WORLD_H_
#include "entity.h"
#include "core/resource/terrain.h"
#include "core/resource/sky.h"
#include "core/rendering/light.h"
#include "core/resource/billboard.h"

#include <vector>

namespace Seed {
class World {
    private:
        std::vector<Entity *> entities;
        std::vector<Ref<Billboard>> billboards;
        Ref<Terrain> terrain;
        Ref<Sky> sky;
        Vec3 ambient_light;
        DirectionalLight direction_light;
        std::vector<PointLight> point_lights;
        Camera camera;

    public:
        std::vector<Entity *> &get_entities();
        Ref<Terrain> get_terrain();
        Ref<Sky> get_sky();
        Vec3 get_ambient_light() { return ambient_light; }
        DirectionalLight &get_direction_light() { return direction_light; }
        std::vector<PointLight> &get_point_lights() { return point_lights; }
        Camera &get_camera() { return camera; }
        void tick(f32 dt);
        void add_entity(Entity *entity);
        void set_terrain(Ref<Terrain> terrain);
        void set_sky(Ref<Sky> sky);

        template <typename T, typename... Args>
        void add_entity(const Args &...args) {
            add_entity(new T(args...));
        }

        void add_billboard(Ref<Billboard> billboard);
        World(/* args */);
        ~World() = default;
};

}  // namespace Seed

#endif