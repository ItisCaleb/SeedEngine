import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position: SV_Position;
}

struct Property{
    Sampler2D height_map; 
    Sampler2D terrain_shadowMap;
    Sampler2D splat_map;
    Sampler2D shadowMap;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(float2 aPos, float2 aTexCoord, uint id: SV_VulkanInstanceID){
    VS_OUT out;
    vec4 terrain = GetTerrain(id);
    out.sv_position = vec4(aPos.x + terrain.x, 0.0f, aPos.y + terrain.y, 1.0);
    out.uv = aTexCoord + terrain.zw;
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

PatchTessFactors PatchConstantFunction() {
    PatchTessFactors factors;
    // Set tessellation levels
    factors.tessFactor[0] = 32;
    factors.tessFactor[1] = 32;
    factors.tessFactor[2] = 32;
    factors.tessFactor[3] = 32;
    factors.insideTessFactor[0] = 32;
    factors.insideTessFactor[1] = 32;
    return factors;
}

struct DS_OUT{
    float4 position : SV_Position;
    float height;
    vec3 normal;
    vec4 fragPos;
    vec2 texCoord;
    float view_depth;
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
    float offset = 0.0001;
    // lookup texel at patch coordinate for height and scale + shift as desired
    out.height = get_height(out.texCoord);
    float rh = get_height(out.texCoord + vec2(offset, 0));
    float lh = get_height(out.texCoord + vec2(-offset, 0));
    float uh = get_height(out.texCoord + vec2(0, offset));
    float dh = get_height(out.texCoord + vec2(0, -offset));

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
    Camera cam = GetCamera();
    //https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
    out.normal = normalize(vec3(lh - rh, 2, dh - uh));
    out.fragPos = p;
    out.view_depth = -(cam.u_view * p).z;
    out.position = cam.u_projection * cam.u_view * p;
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

[shader("fragment")]
float4 frag(DS_OUT in){
    float h = (in.height + 128)/256.0f;
    if(h < 0.1){
        discard;
    }

    vec3 light_out = gLights.u_light_ambient;
    Camera cam = GetCamera();
    vec3 view_dir = normalize(cam.u_cam_pos - in.fragPos.xyz);
    vec3 diffuse_sample = vec3(1,1,1);
    vec3 specular_sample = vec3(1,1,1);
    vec3 dir_light = -normalize(vec3(gLights.u_dir_light.position));
    float c = dir_light.y;

    float csm_shadow = CSMShadowCalculation(prop.shadowMap, in.fragPos, in.view_depth, in.normal, dir_light);
    float horizon_shadow = HorizonShadow(in.texCoord, dir_light);
    float shadow = max(csm_shadow, horizon_shadow);
    
    // direction
    light_out +=
        calculate_light(gLights.u_dir_light.diffuse, gLights.u_dir_light.specular,
            diffuse_sample, specular_sample, dir_light, view_dir,
            1, in.normal) * (1.0 - shadow);
    for (int i = 0; i < 8; i++) {
        Light light = gLights.u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = vec3(light.position) - in.fragPos.xyz;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
            diffuse_sample, specular_sample,
            light_dir, view_dir, d, in.normal);
    }
    light_out *= vec3(h, h, h);
    float dist = length(cam.u_cam_pos - in.fragPos.xyz);
    float fog_factor = clamp((dist - 700) / 100, 0.0, 0.95);
    vec3 final_color = lerp(light_out, vec3(0.48, 0.80, 0.80), fog_factor);

    return vec4(final_color, 1.0);
}