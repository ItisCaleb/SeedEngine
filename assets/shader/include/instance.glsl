layout (location = 8) in uint aInstanceIndex;

layout(std430, binding = 0) buffer TransformInstanceDatas
{
    mat4 b_transform[];
};

layout(std430, binding = 1) buffer TerrainInstanceDatas
{
    vec4 b_terrain[];
};