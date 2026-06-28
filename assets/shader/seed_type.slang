typedef float2 vec2
typedef float3 vec3
typedef float4 vec4 
typedef float3x3 mat3
typedef float4x4 mat4


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