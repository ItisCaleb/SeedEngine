#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexCoord;

#include <instance.glsl>
#include <phong_lighting.glsl>
#include <shadow.glsl>

layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

layout(std140) uniform Camera { vec3 u_cam_pos; };


out vec2 texCoord;
out vec3 fragPos;
out vec3 view_pos;
out float view_depth;
out vec3 tangent_dir_light;
out vec3 tangent_pos_light[8];


void main(){
    mat4 aModel = b_transform[aInstanceIndex];
    vec4 worldPos = aModel * vec4(aPos, 1.0);
    // For right handed system,
    // after view transformation, the depth is always negative
    // So we negate it
    view_depth = -(u_view * worldPos).z;
    gl_Position = u_projection * u_view * worldPos;

    mat3 TBN_to_world = mat3(1.0);
    if(length(aNormal) > 0.01){
        vec3 T = normalize(vec3(aModel * vec4(aTangent.xyz, 0.0)));
        vec3 N = normalize(vec3(aModel * vec4(aNormal, 0.0)));
        T = normalize(T - dot(T, N) * N);  // Gram-Schmidt
        vec3 B = cross(N, T);
        TBN_to_world = mat3(T, B, N);
    }

    mat3 TBN_to_tangent = transpose(TBN_to_world);

    texCoord = aTexCoord;

    fragPos = TBN_to_tangent * vec3(worldPos);
    view_pos = TBN_to_tangent * u_cam_pos;
    tangent_dir_light = TBN_to_tangent * (u_dir_light.position);

    for (int i = 0; i < 8; i++) {
        Light light = u_point_lights[i];
        if (light.enable == 0)
            continue;
        tangent_pos_light[i] = TBN_to_tangent * (light.position); 
    }

}