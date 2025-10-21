#version 450 core
layout (location = 0) in vec3 aPos;

#include <instance.glsl>

void main(){
    mat4 aModel = b_transform[aInstanceIndex];
    gl_Position = aModel * vec4(aPos, 1.0);
}