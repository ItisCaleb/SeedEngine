import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position: SV_Position;
    nointerpolation uint heightmap_index;
}

struct Property{
    Sampler2DArray height_map; 
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(float2 aPos, uint id: SV_VulkanInstanceID){
    VS_OUT out;
    TerrainInstance terrain = GetTerrain(id);
    vec2 size = 256; 
    out.sv_position = vec4(aPos.x + terrain.pos.x, 0.0f, aPos.y + terrain.pos.y, 1.0);
    out.uv = (out.sv_position.xz + size / 2)/size;
    out.heightmap_index = terrain.heightmap_index;
    return out;
}

struct HSOutput
{
    float4 position;
    float2 uv;
    nointerpolation uint heightmap_index;
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
    o.heightmap_index = patch[cpID].heightmap_index;
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
    float2 map_pos;
    float height;
    vec3 normal;
    vec4 fragPos;
    vec2 texCoord;
};


float get_height(uint layer, vec2 tex_coord){
    return prop.height_map.SampleLevelZero(vec3(tex_coord, layer)).y * 256.0 - 128.0;
}

// float SampleHorizonShadow(vec2 uv, vec3 light_dir){
//     // lookup texel at patch coordinate for height and scale + shift as desired
//     float4 slopes = prop.terrain_shadowMap.SampleLevelZero(uv);

//     float sx = light_dir.x < 0 ? slopes.r : slopes.g;
//     float sz = light_dir.z < 0 ? slopes.b : slopes.a;

//     float wx = abs(light_dir.x);
//     float wz = abs(light_dir.z);
//     float total = wx + wz;  // L1 norm
//     float slope = (sx * wx + sz * wz) / total;
//     float angle = light_dir.y;
//     float soft_width = 0.15;
//     float shadow = smoothstep(angle - soft_width,
//                               angle + soft_width,
//                               slope);

//     return shadow;
// }

// float HorizonShadow(vec2 uv, vec3 light_dir) {
//     float shadow = 0.0;
//     uint level;
//     uint2 size;
//     prop.terrain_shadowMap.GetDimensions(0u, size[0], size[1], level);
//     vec2 tex_scale = (1.0 / vec2(size));
    
//     int kernel_size = 3;
//     /* poisson disk */
//     for (int i = 0; i < 16; i++) {
//         vec2 sample_offset = poisson_disk[i] * kernel_size * tex_scale;
//         shadow += SampleHorizonShadow(uv + sample_offset, light_dir);
//     }
//     shadow /= 16.0;
//     return shadow;
// }

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
    uint index = patch[0].heightmap_index;
    out.height = get_height(index,out.texCoord);
    float rh = get_height(index,out.texCoord + vec2(offset, 0));
    float lh = get_height(index,out.texCoord + vec2(-offset, 0));
    float uh = get_height(index,out.texCoord + vec2(0, offset));
    float dh = get_height(index,out.texCoord + vec2(0, -offset));

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
    out.map_pos = vec2(p.x, p.z);
    // displace point along normal
    p += vec4(0, out.height, 0 , 0);
    
    //https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
    out.normal = normalize(vec3(lh - rh, 2, dh - uh));
    out.fragPos = p;

    Camera cam = GetCamera();
    out.position = cam.u_projection * cam.u_view * p;
    // ----------------------------------------------------------------------
    // output patch point position in clip space
    return out;
}

struct FS_OUT{
    vec4 albedo: SV_Target0;
    int4 position: SV_Target1;
};

[shader("fragment")]
FS_OUT frag(DS_OUT in){
    FS_OUT out;
    float h = (in.height + 128)/256.0f;

    vec3 light_out = vec3(0.3, 0.3, 0.3);
    vec3 view_dir = normalize(vec3(0.3,0.3,0.3));
    vec3 diffuse_sample = vec3(1,1,1);
    vec3 specular_sample = vec3(1,1,1);
    vec3 dir_light = -normalize(vec3(-0.5,-0.5,0));
    //float horizon_shadow = HorizonShadow(in.texCoord, dir_light);
    // direction
    light_out +=
        calculate_light(gLights.u_dir_light.diffuse, gLights.u_dir_light.specular,
            diffuse_sample, specular_sample, dir_light, view_dir,
            1, in.normal);
    //light_out *=  (1.0 - horizon_shadow);
    light_out *= vec3(h, h, h);
    out.albedo = vec4(light_out, 1.0);

    out.position = int4(
        (int)(in.map_pos.x),
        (int)(in.map_pos.y), 0, 0
    );
    return out; 
}