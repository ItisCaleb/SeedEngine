#ifndef _SEED_WORLD_SETTING_H_
#define _SEED_WORLD_SETTING_H_

#include "core/container/kstring.h"
#include "core/misc/uuid.h"
#include "core/math/vec3.h"
#include "resource.h"
namespace Seed {

struct SkySetting {
        UUID up;
        UUID down;
        UUID left;
        UUID right;
        UUID front;
        UUID back;
};
struct DirectionalLightSetting {
        Vec3 direction;
        Vec3 diffuse;
        Vec3 specular;
};

struct PointLightSetting {
        Vec3 position;
        Vec3 diffuse;
        Vec3 specular;
};

struct StaticObjectSetting {
        KString name;
        i32 x = 0;
        i32 y = 0;
        UUID model;
};

struct ChunkSetting {
        i32 x = 0;
        i32 y = 0;
        UUID height_map;
        UUID control_map;
        std::vector<PointLightSetting> lights;
        std::vector<StaticObjectSetting> static_objects;
};

struct WorldSetting : public Resource {
        KString name;
        SkySetting sky;
        DirectionalLightSetting dir_light;
        std::vector<ChunkSetting> chunks;
        std::vector<UUID> terrain_textures;
        std::vector<UUID> terrain_normals;
};

}  // namespace Seed

#endif