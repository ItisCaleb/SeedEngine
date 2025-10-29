#version 450 core
layout (location = 0) in vec3 aPos;

#include <instance.glsl>

layout (std140) uniform CSMShadow
{
    mat4 u_lightspaces[4];
    vec4 u_shadow_uv[4];
    vec4 u_far;
    vec4 u_shadow_unit;
};

void main(){
    mat4 aModel = b_transform[aInstanceIndex];
    gl_Position = u_lightspaces[0] * aModel * vec4(aPos, 1.0);
}