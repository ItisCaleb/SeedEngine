#include "seed_type.slang"

[[vk::binding(0)]]
StructuredBuffer<uint> gVisible;

[[vk::binding(1)]]
StructuredBuffer<mat4> gTransform;


struct TerrainInstance{
    vec2 pos;
    uint heightmap_index;
    uint dummy;
};

[[vk::binding(2)]]
StructuredBuffer<TerrainInstance> gTerrain;

[[vk::binding(3)]]
StructuredBuffer<mat4> gBones;

struct Camera {
    vec3 u_cam_pos;
    mat4 u_projection;
    mat4 u_view;
}

struct Light {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    float enable;
};
struct Lights {
    vec3 u_light_ambient;
    Light u_dir_light;
    Light u_point_lights[8];
};

static const int CSM_SPLITS = 4;
struct CSMShadow
{
    /* x, y, w, h */
    vec4 u_shadow_uv[CSM_SPLITS];
    vec4 u_far;
    vec4 u_shadow_unit;
};


[[vk::binding(8)]]
ConstantBuffer<Camera[64]> gCam;
[[vk::binding(9)]]
ConstantBuffer<Lights> gLights;
[[vk::binding(10)]]
ConstantBuffer<CSMShadow> gCSM;
[[vk::binding(11)]]
ConstantBuffer<mat4> gProj;