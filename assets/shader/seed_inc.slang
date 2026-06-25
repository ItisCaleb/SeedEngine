typedef float2 vec2
typedef float3 vec3
typedef float4 vec4 
typedef float3x3 mat3
typedef float4x4 mat4 

[[vk::binding(0)]]
StructuredBuffer<uint> gVisible;

struct PushConstant {
    uint visible_index;
    /* current camera to use */
    uint camera;
}
[[vk_push_constant]]
ConstantBuffer<PushConstant> gPC;

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

#define CSM_SPLITS 4
struct CSMShadow
{
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

inline TerrainInstance GetTerrain(uint id){
    return gTerrain[gVisible[id + gPC.visible_index]];
}

inline mat4 GetSkeletonTransform(uint id){
    return gBones[gVisible[id + gPC.visible_index]];
}

inline mat4 GetSkeletonBone(uint id, uint bone_id){
    return gBones[gVisible[id + gPC.visible_index] + 1 + bone_id];
}

inline Camera GetCamera(){
    return gCam[gPC.camera];
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

float ShadowBilinear(Sampler2D shadow_map, vec2 uv, vec2 tex_scale, float sample_depth){
    vec2 size = 1/tex_scale;
    vec2 p = uv * size;
    vec2 i0 = floor(p);
    vec2 f = frac(p);
    vec2 i1 = i0 + 1.0;

    i0 = clamp(i0, vec2(0), vec2(size - 1.0));
    i1 = clamp(i1, vec2(0), vec2(size - 1.0));

    vec2 uv00 = (vec2(i0.x, i0.y)) / size;
    vec2 uv01 = (vec2(i0.x, i1.y)) / size;
    vec2 uv10 = (vec2(i1.x, i0.y)) / size;
    vec2 uv11 = (vec2(i1.x, i1.y)) / size;

    float s00 = sample_depth > shadow_map.Sample(uv00).r ? 1.0 : 0.0;
    float s01 = sample_depth > shadow_map.Sample(uv01).r ? 1.0 : 0.0;
    float s10 = sample_depth > shadow_map.Sample(uv10).r ? 1.0 : 0.0;
    float s11 = sample_depth > shadow_map.Sample(uv11).r ? 1.0 : 0.0;

    return lerp(lerp(s00, s01, f.y), lerp(s10, s11, f.y), f.x);
}


float ShadowAverage(Sampler2D shadow_map, vec2 uv, vec2 tex_scale, float sample_depth){
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
    int casacade = CSM_SPLITS;
    for(int i = CSM_SPLITS - 1;i >= 0;i--){
        if(depth < gCSM.u_far[i]) casacade = i;
    }
    return casacade;
}

float ShadowPCF(Sampler2D shadow_map, vec2 projCoords, float depth, float kernel_size, vec2 tex_scale){
    float o = (kernel_size - 1) / 2;
    float x, y;
    float shadow = 0;
    int sample_count = 0;
    for(y = -o; y <= o; y += 1.0){
        for (x = -o;x <= o; x += 1.0){
            vec2 sample_offset = vec2(x, y) * tex_scale;
            shadow += ShadowAverage(shadow_map,
                projCoords + sample_offset, tex_scale, depth);
            sample_count++;
            // float _sample = texture(shadow_map, projCoords.xy + sample_offset).r;
            // shadow += real_depth > _sample ? 1.0 : 0.0;
        }
    }
    shadow /= float(sample_count);
    return shadow;
}

static const float2 poisson_disk[] = {
   float2( -0.94201624, -0.39906216 ), 
   float2( 0.94558609, -0.76890725 ), 
   float2( -0.094184101, -0.92938870 ), 
   float2( 0.34495938, 0.29387760 ), 
   float2( -0.91588581, 0.45771432 ), 
   float2( -0.81544232, -0.87912464 ), 
   float2( -0.38277543, 0.27676845 ), 
   float2( 0.97484398, 0.75648379 ), 
   float2( 0.44323325, -0.97511554 ), 
   float2( 0.53742981, -0.47373420 ), 
   float2( -0.26496911, -0.41893023 ), 
   float2( 0.79197514, 0.19090188 ), 
   float2( -0.24188840, 0.99706507 ), 
   float2( -0.81409955, 0.91437590 ), 
   float2( 0.19984126, 0.78641367 ), 
   float2( 0.14383161, -0.14100790 ) 
};

float ShadowPoissonDisk(Sampler2D shadow_map, vec2 projCoords, float depth, float kernel_size, vec2 tex_scale){
    float shadow = 0.0;
    for (int i = 0; i < 16; i++) {
        vec2 sample_offset = poisson_disk[i] * kernel_size * tex_scale;
        shadow += ShadowBilinear(shadow_map, projCoords.xy + sample_offset, tex_scale, depth);
    }
    shadow /= 16.0;
    return shadow;
}

float CalculatePCSSPenumbra(Sampler2D shadow_map, vec3 projCoords, float depth, vec2 tex_scale){
    float blockerDepth = 0.0;
    int blockerCount = 0;
    /* 0.02 for directional light */
    float light_size = 0.02;
    float searchRadius = light_size * (depth - 0.1) / depth;
    for (int i = 0; i < 16; i++) {
        vec2 offset = poisson_disk[i] * searchRadius * tex_scale;
        float shadowDepth = shadow_map.SampleLevel(projCoords.xy + offset, 0).r;
        if (shadowDepth < depth) {
            blockerDepth += shadowDepth;
            blockerCount++;
        }
    }

    if (blockerCount == 0) return 0.0;
    blockerDepth /= float(blockerCount);

    float penumbraWidth = (depth - blockerDepth) / blockerDepth * light_size;
    return penumbraWidth;
}


float SampleCasacade(Sampler2D shadow_map, int casacade, vec4 frag_pos, vec3 normal, vec3 light_dir){
    /* index 0 is main camera */
    Camera cam = gCam[1 + casacade];
    vec3 n = normalize(normal);
    vec4 offset = gCSM.u_shadow_unit[casacade] * vec4(shadow_pos_offset(shadow_map, dot(n, light_dir), n, 2), 0.0);
    vec4 lightspace_fragpos = cam.u_projection * cam.u_view * (frag_pos + offset);
    // perform perspective divide
    vec3 projCoords = lightspace_fragpos.xyz / lightspace_fragpos.w;
    //vec2 depth_duv = recieve_plane_bias_uv(projCoords);
    // transform to [0,1] range
    // Vulkan's clip space z is already [0,1]
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    // get closest depth value from light’s perspective (using
    // [0,1] range fragPosLight as coords)
    // shadow map atlas transform
    projCoords.xy = projCoords.xy * gCSM.u_shadow_uv[casacade].zw + gCSM.u_shadow_uv[casacade].xy;
    uint2 size;
    uint level;
    shadow_map.GetDimensions(0u, size[0], size[1], level);
    vec2 tex_scale = (1.0 / vec2(size));
    float ndotl = max(dot(normal, light_dir), 0.0);
    float bias = max(0.0005, 0.05 * (1.0 - ndotl));
    float currentDepth = projCoords.z - bias;
    // float penumbra = CalculatePCSSPenumbra(shadow_map, projCoords, currentDepth, tex_scale);
    // if(penumbra == 0.0) return 0.0f;

    // float shadow = ShadowPCF(shadow_map, projCoords.xy, currentDepth, 3.0, tex_scale);
    float shadow = ShadowPoissonDisk(shadow_map, projCoords.xy, currentDepth, 3.0, tex_scale);

    return shadow;
}

float CSMShadowCalculation(Sampler2D shadow_map, vec4 frag_pos, float frag_z, vec3 normal, vec3 light_dir)
{
    int casacade_idx = SelectCasacade(frag_z);
    if(casacade_idx == CSM_SPLITS) return 0.0;
    float shadow = SampleCasacade(shadow_map, casacade_idx, frag_pos, normal, light_dir);
    float cascade_end = gCSM.u_far[casacade_idx];
    float blend_range = cascade_end * 0.1;
    float blend_start = cascade_end - blend_range;
    if(frag_z > blend_start && casacade_idx < 3) {
        float shadow1 = SampleCasacade(shadow_map, casacade_idx + 1, frag_pos, normal, light_dir);
        float t = (frag_z - blend_start) / blend_range;
        shadow = lerp(shadow, shadow1, t);
    }
    return shadow;    
}

vec3 sample_normal(vec2 uv, Sampler2D normal_map){
    return normalize(normal_map.Sample(uv).rgb * 2.0 - 1.0);
}


vec3 blend_normal_rnm(vec3 n1, vec3 n2) {
    vec3 t = n1 * vec3(2, 2, 2) + vec3(-1, -1, 0);
    vec3 u = n2 * vec3(-2, -2, 2) + vec3(1, 1, -1);
    vec3 r = t * dot(t, u) - u * t.z;
    return normalize(r);
}