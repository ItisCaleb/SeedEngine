#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

#include <instance.glsl>

layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

layout(std140) uniform Camera { vec3 u_cam_pos; };

out vec2 texCoord;

void main(){
    mat4 aModel = b_transform[aInstanceIndex];
    vec3 scale = vec3(aModel[0][0], aModel[1][1], aModel[2][2]);
    vec3 pos = aModel[3].xyz;

    // calculate camera params
    vec3 up = vec3(0, 1, 0);
    vec3 front = normalize(pos - u_cam_pos);
    vec3 right = normalize(cross(front, up));

    vec3 worldPos = pos + right * aPos.x * scale.x + up * aPos.y * scale.y;
    texCoord = aTexCoord;
    gl_Position = u_projection * u_view * vec4(worldPos, 1.0);
}