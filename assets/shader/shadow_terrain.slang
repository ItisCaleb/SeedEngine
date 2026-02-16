import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position : SV_Position;
}

struct Property{
    Sampler2D height_map; 
    Sampler2D terrain_shadow_map;
};

ParameterBlock<Property> prop;


[shader("vertex")]
VS_OUT vert(float2 aPos, float2 aTexCoord, uint id: SV_VulkanInstanceID){
    VS_OUT out;
    vec4 terrain = gTerrain[gVisible[id]];
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

PatchTessFactors PatchConstantFunction(InputPatch<VS_OUT, 4> patch) {
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


float get_height(vec2 tex_coord){
    return prop.height_map.SampleLevelZero(tex_coord).y * 256.0 - 128.0;
}

[[vk_push_constant]]
ConstantBuffer<uint> split;

[shader("domain")]
[domain("quad")]
[partitioning("fractional_odd")]
vec4 tese(
    float3 tessCoord : SV_DomainLocation,
    const OutputPatch<HSOutput, 4> patch
) : SV_Position{
    // get patch coordinate
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
    vec2 texCoord = (t1 - t0) * v + t0;
    float offset = 0.0001;
    // lookup texel at patch coordinate for height and scale + shift as desired
    float height = get_height(texCoord);
    float rh = get_height(texCoord + vec2(offset, 0));
    float lh = get_height(texCoord + vec2(-offset, 0));
    float uh = get_height(texCoord + vec2(0, offset));
    float dh = get_height(texCoord + vec2(0, -offset));

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
    p += vec4(0, height, 0 , 0);

    return gCSM.u_lightspaces[split] * p;
}