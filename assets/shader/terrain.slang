import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position: SV_Position;
}


struct Property{
    Sampler2D height_map; 
    Sampler2D terrain_shadowMap;
    Sampler2D splat_map;
    Sampler2D tex1;
    Sampler2D tex1_normal;
    Sampler2D shadowMap;
    vec2 terrain_size;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(float2 aPos, uint id: SV_VulkanInstanceID){
    VS_OUT out;
    TerrainInstance terrain = GetTerrain(id);
    vec2 size = prop.terrain_size; 
    out.sv_position = vec4(aPos.x + terrain.pos.x, 0.0f, aPos.y + terrain.pos.y, 1.0);
    out.uv = (out.sv_position.xz + size / 2)/size;
    return out;
}

struct HSOutput
{
    float4 position;
    float2 uv;
};

[shader("hull")]
[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PatchConstantFunction")]
HSOutput tesc(InputPatch<VS_OUT, 4> patch, uint cpID : SV_OutputControlPointID){
    HSOutput o;
    o.position = patch[cpID].sv_position;
    o.uv = patch[cpID].uv;
    return o;
}

struct PatchTessFactors {
    float4 tessFactor : SV_TessFactor;
    float2 insideTessFactor : SV_InsideTessFactor;
};

PatchTessFactors PatchConstantFunction(InputPatch<VS_OUT, 4> patch) {
    PatchTessFactors factors;

    const uint min_tess = 2;
    const uint max_tess = 32;
    const uint min_dist = 20;
    const uint max_dist = 700;
    const uint range = max_dist - min_dist;

    Camera cam = GetCamera();
    vec4 cam_pos00 = cam.u_view * patch[0].sv_position;
    vec4 cam_pos01 = cam.u_view * patch[1].sv_position;
    vec4 cam_pos10 = cam.u_view * patch[2].sv_position;
    vec4 cam_pos11 = cam.u_view * patch[3].sv_position;

    float dist00 = clamp((abs(cam_pos00.z) - min_dist) / range, 0, 1);
    float dist01 = clamp((abs(cam_pos01.z) - min_dist) / range, 0, 1);
    float dist10 = clamp((abs(cam_pos10.z) - min_dist) / range, 0, 1);
    float dist11 = clamp((abs(cam_pos11.z) - min_dist) / range, 0, 1);

    // Set tessellation levels
    factors.tessFactor[0] = lerp(max_tess, min_tess, min(dist10, dist00));
    factors.tessFactor[1] = lerp(max_tess, min_tess, min(dist00, dist01));
    factors.tessFactor[2] = lerp(max_tess, min_tess, min(dist01, dist11));
    factors.tessFactor[3] = lerp(max_tess, min_tess, min(dist11, dist10));
    factors.insideTessFactor[0] = max(factors.tessFactor[1], factors.tessFactor[3]);
    factors.insideTessFactor[1] = max(factors.tessFactor[0], factors.tessFactor[2]);
    return factors;
}

struct DS_OUT{
    float4 position : SV_Position;
    float height;
    vec4 frag_pos;
    vec3 normal;
    vec3 tangent_frag_pos;
    vec3 tangent_view_pos;
    vec2 texCoord;
    float view_depth;
    vec3 tangent_dir_light;
    vec3 tangent_pos_light[8];
};


float get_height(vec2 tex_coord){
    return prop.height_map.SampleLevelZero(tex_coord).y * 256.0 - 128.0;
}

[shader("domain")]
[domain("quad")]
[partitioning("fractional_odd")]
DS_OUT tese(
    float3 tessCoord : SV_DomainLocation,
    const OutputPatch<HSOutput, 4> patch
){
    // get patch coordinate
    DS_OUT out;
    float u = tessCoord.x;
    float v = tessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates
    vec2 t00 = patch[0].uv;
    vec2 t01 = patch[1].uv;
    vec2 t10 = patch[2].uv;
    vec2 t11 = patch[3].uv;

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    out.texCoord = (t1 - t0) * v + t0;
    vec2 offset; 
    float level;
    prop.height_map.GetDimensions(0, offset[0], offset[1], level);
    offset = 1 / offset;
    // lookup texel at patch coordinate for height and scale + shift as desired
    float h = get_height(out.texCoord);
    float rh = get_height(out.texCoord + vec2(offset.x, 0));
    float lh = get_height(out.texCoord + vec2(-offset.x, 0));
    float uh = get_height(out.texCoord + vec2(0, offset.y));
    float dh = get_height(out.texCoord + vec2(0, -offset.y));
    out.height = (h + rh + lh + uh + dh) / 5;
    // ----------------------------------------------------------------------
    // retrieve control point position coordinates
    vec4 p00 = patch[0].position;
    vec4 p01 = patch[1].position;
    vec4 p10 = patch[2].position;
    vec4 p11 = patch[3].position;

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    // displace point along normal
    p += vec4(0, out.height, 0 , 0);
    vec3 normal = normalize(vec3(lh - rh, 2, dh - uh));
    vec3 bitangent = cross(vec3(0,0,1), normal);
    vec3 tangent = cross(normal, bitangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    Camera cam = GetCamera();
    //https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
    out.frag_pos = p;
    out.tangent_frag_pos = TBN * p.xyz;
    out.normal = normal;
    out.view_depth = -(cam.u_view * p).z;
    out.position = cam.u_projection * cam.u_view * p;
    out.tangent_view_pos = TBN * cam.u_cam_pos;
    out.tangent_dir_light = TBN * (gLights.u_dir_light.position);

    for (int i = 0; i < 8; i++) {
        Light light = gLights.u_point_lights[i];
        if (light.enable == 0)
            continue;
        out.tangent_pos_light[i] = TBN * (light.position); 
    }
    // ----------------------------------------------------------------------
    // output patch point position in clip space
    return out;
}


float SampleHorizonShadow(vec2 uv, vec3 light_dir){
    // lookup texel at patch coordinate for height and scale + shift as desired
    float4 slopes = prop.terrain_shadowMap.SampleLevelZero(uv);

    float sx = light_dir.x < 0 ? slopes.r : slopes.g;
    float sz = light_dir.z < 0 ? slopes.b : slopes.a;

    float wx = abs(light_dir.x);
    float wz = abs(light_dir.z);
    float total = wx + wz;  // L1 norm
    float slope = (sx * wx + sz * wz) / total;
    float angle = light_dir.y;
    float soft_width = 0.15;
    float shadow = smoothstep(angle - soft_width,
                              angle + soft_width,
                              slope);

    return shadow;
}

float HorizonShadow(vec2 uv, vec3 light_dir) {
    float shadow = 0.0;
    uint level;
    uint2 size;
    prop.terrain_shadowMap.GetDimensions(0u, size[0], size[1], level);
    vec2 tex_scale = (1.0 / vec2(size));
    
    int kernel_size = 3;
    /* poisson disk */
    for (int i = 0; i < 16; i++) {
        vec2 sample_offset = poisson_disk[i] * kernel_size * tex_scale;
        shadow += SampleHorizonShadow(uv + sample_offset, light_dir);
    }
    shadow /= 16.0;
    return shadow;
}

vec3 diffuse_blend(vec2 uv1, vec2 uv2){
    vec3 diffuse1 = prop.tex1.Sample(uv1).xyz;
    vec3 diffuse2 = prop.tex1.Sample(uv2).xyz;
    return diffuse1 * diffuse2 * 2.0;
}

vec3 normal_blend(vec2 uv1, vec2 uv2){
    vec3 n1 = sample_normal(uv1, prop.tex1_normal); 
    vec3 n2 = sample_normal(uv2, prop.tex1_normal); 
    return blend_normal_rnm(n1, n2);
}

vec3 triplanar_mapping(vec3 frag_pos, vec3 world_normal, Sampler2D texture, float scale){
    vec2 uv_x = frag_pos.zy / scale;
    vec2 uv_y = frag_pos.xz / scale;
    vec2 uv_z = frag_pos.xy / scale;
    vec3 weights = abs(world_normal);
    weights = weights / (weights.x + weights.y + weights.z);
    vec3 sx = texture.Sample(uv_x).xyz;
    vec3 sy = texture.Sample(uv_y).xyz;
    vec3 sz = texture.Sample(uv_z).xyz;
    vec3 sampled = sx * weights.x + sy * weights.y + sz * weights.z;
    return sampled;
}

[shader("fragment")]
float4 frag(DS_OUT in){
    float h = (in.height + 128)/256.0f;
    if(h < 0.1){
        discard;
    }
    vec2 uv1 = in.frag_pos.xz/32;
    vec2 uv2 = in.frag_pos.xz/128;
    // vec3 diffuse_sample = diffuse_blend(uv1, uv2);
    // vec3 normal = normal_blend(uv1, uv2);
    vec3 world_normal = normalize(in.normal);
    vec3 diffuse_sample = triplanar_mapping(in.frag_pos.xyz, world_normal, prop.tex1, 32);
    vec3 normal = sample_normal(uv1,  prop.tex1_normal);

    vec3 light_out = diffuse_sample * gLights.u_light_ambient;
    Camera cam = GetCamera();
    vec3 view_dir = normalize(in.tangent_view_pos - in.tangent_frag_pos.xyz);
    vec3 specular_sample = vec3(1,1,1);
    vec3 dir_light = -normalize(vec3(gLights.u_dir_light.position));
    vec3 tangent_dir_light = -normalize(in.tangent_dir_light);

    float csm_shadow = CSMShadowCalculation(prop.shadowMap, in.frag_pos, in.view_depth, world_normal, dir_light);
    float horizon_shadow = HorizonShadow(in.texCoord, dir_light);
    float shadow = max(csm_shadow, horizon_shadow);
    
    // direction
    light_out +=
        calculate_light(gLights.u_dir_light.diffuse, gLights.u_dir_light.specular,
            diffuse_sample, specular_sample, tangent_dir_light, view_dir,
            1, normal) * (1.0 - shadow);
    for (int i = 0; i < 8; i++) {
        Light light = gLights.u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = in.tangent_pos_light[i] - in.tangent_frag_pos.xyz;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
            diffuse_sample, specular_sample,
            light_dir, view_dir, d, normal);
    }
    light_out *= vec3(h, h, h);
    float dist = length(cam.u_cam_pos - in.frag_pos.xyz);
    float fog_factor = clamp((dist - 700) / 100, 0.0, 0.95);
    vec3 final_color = lerp(light_out, vec3(0.48, 0.80, 0.80), fog_factor);

    return vec4(final_color, 1.0);
}