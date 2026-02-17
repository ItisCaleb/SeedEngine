typedef float2 vec2
typedef float3 vec3
typedef float4 vec4 
typedef float3x3 mat3
typedef float4x4 mat4 

[[vk::binding(0)]]
StructuredBuffer<uint> gVisible;

struct PushConstant {
    uint visible_index;
    /* used by CSM */
    uint split;
}
[[vk_push_constant]]
ConstantBuffer<PushConstant> gPC;

[[vk::binding(1)]]
StructuredBuffer<mat4> gTransform;

[[vk::binding(2)]]
StructuredBuffer<vec4> gTerrain;

[[vk::binding(3)]]
StructuredBuffer<mat4> gBones;

struct Camera {
    vec3 u_cam_pos;
}
struct Matrices{
    mat4 u_projection;
    mat4 u_view;
    mat4 u_screen_projection;
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
    //Light u_point_lights[8];
};

struct CSMShadow
{
    mat4 u_lightspaces[4];
    vec4 u_shadow_uv[4];
    vec4 u_far;
    vec4 u_shadow_unit;
};


[[vk::binding(8)]]
ConstantBuffer<Camera> gCam;
[[vk::binding(9)]]
ConstantBuffer<Matrices> gMatrices;
[[vk::binding(10)]]
ConstantBuffer<Lights> gLights;
[[vk::binding(11)]]
ConstantBuffer<CSMShadow> gCSM;



inline vec4 operator*(mat4 a, vec4 b)
{
    return mul(b, a);
}

inline vec3 operator*(mat3 a, vec3 b)
{
    return mul(b, a);
}

inline mat4 operator*(mat4 a, mat4 b)
{
    return mul(b, a);
}


inline mat4 GetTransform(uint id){
    return gTransform[gVisible[id + gPC.visible_index]];
}

inline vec4 GetTerrain(uint id){
    return gTerrain[gVisible[id + gPC.visible_index]];
}

inline mat4 GetBone(uint id){
    return gBones[gVisible[id + gPC.visible_index]];
}

inline mat4 GetLightSpace(){
    return gCSM.u_lightspaces[gPC.split];
}

vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 diffuse_sample, vec3 specular_sample, vec3 light_dir, vec3 view_dir,
                     float d, vec3 n) {
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * diffuse_sample * diff;
    vec3 specular_l = specular * specular_sample * spec;

    return att * (diffuse_l + specular_l);
}


vec3 shadow_pos_offset(Sampler2D shadow_map, float n_dot_l, vec3 normal, float scale){
    float offset_scale = clamp(1 - n_dot_l, 0.0, 1.0);
    return offset_scale * scale * normal;
}

float shadow_bilinear(Sampler2D shadow_map, vec2 uv, vec2 tex_scale, float sample_depth){
    float r = shadow_map.Sample(uv + vec2(1,0) * tex_scale).r;
    float l = shadow_map.Sample(uv + vec2(-1,0) * tex_scale).r;
    float t = shadow_map.Sample(uv + vec2(0,1) * tex_scale).r;
    float b = shadow_map.Sample(uv + vec2(0,-1) * tex_scale).r;
    r = sample_depth > r ? 1.0 : 0.0;
    l = sample_depth > l ? 1.0 : 0.0;
    t = sample_depth > t ? 1.0 : 0.0;
    b = sample_depth > b ? 1.0 : 0.0;
    return (l + r + t + b) * 0.25;
}

// vec2 recieve_plane_bias_uv(vec3 projCoords){
//     vec3 pc_dx = ddx_fine(projCoords);
//     vec3 pc_dy = ddy_fine(projCoords);
//     vec2 depth_duv;
//     depth_duv.x = pc_dy.y * pc_dx.z - pc_dx.y * pc_dy.z;
//     depth_duv.y = pc_dx.x * pc_dy.z - pc_dy.x * pc_dx.z;
//     float inv_det = 1.0 / ((pc_dx.x * pc_dy.y) - (pc_dx.y * pc_dy.x));
//     depth_duv *= inv_det;
//     return depth_duv;
// }

int SelectCasacade(float depth){
    int casacade = 3;
    for(int i = 3;i >= 0;i--){
        if(depth < gCSM.u_far[i]) casacade = i;
    }
    return casacade;
}

float ShadowCalculation(Sampler2D shadow_map, vec4 frag_pos, float frag_z, vec3 normal, vec3 light_dir)
{
    int casacade_idx = SelectCasacade(frag_z);
    if(casacade_idx == 3) return 0.0;
    vec3 n = normalize(normal);
    vec4 offset = gCSM.u_shadow_unit[casacade_idx] * vec4(shadow_pos_offset(shadow_map, dot(n, light_dir), n, 2), 0.0);
    vec4 lightspace_fragpos = gCSM.u_lightspaces[casacade_idx] * (frag_pos + offset);
    // perform perspective divide
    vec3 projCoords = lightspace_fragpos.xyz / lightspace_fragpos.w;
    //vec2 depth_duv = recieve_plane_bias_uv(projCoords);
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // get closest depth value from light’s perspective (using
    // [0,1] range fragPosLight as coords)
    // shadow map atlas transform
    projCoords.xy = projCoords.xy * gCSM.u_shadow_uv[casacade_idx].zw + gCSM.u_shadow_uv[casacade_idx].xy;
    uint2 size;
    uint level;
    shadow_map.GetDimensions(0u, size[0], size[1], level);
    vec2 tex_scale = (1.0 / vec2(size));
    vec4 normal_bias = vec4(0.02, 0.02, 0.03, 0.05);
    float bias = normal_bias[casacade_idx] * dot(normal, light_dir);
    float shadow = 0;
    float currentDepth = projCoords.z - bias;
    float pcf_count = 3.0;
    float o = (pcf_count - 1) / 2;
    float x, y;
    for(y = -o; y <= o; y += 1.0){
        for (x = -o;x <= o; x += 1.0){
            vec2 sample_offset = vec2(x, y) * tex_scale;
            float real_depth = currentDepth;
            shadow += shadow_bilinear(shadow_map,
                projCoords.xy + sample_offset, tex_scale, real_depth);
            // float _sample = texture(shadow_map, projCoords.xy + sample_offset).r;
            // shadow += real_depth > _sample ? 1.0 : 0.0;
        }
    }
    shadow /= (pcf_count) * (pcf_count);
    return shadow;
}