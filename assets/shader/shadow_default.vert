#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in uint aModelIndex;

layout(std430, binding = 0) buffer layoutName
{
    // 2^19
    mat4 b_models[524288];
};


void main(){
    mat4 aModel = b_models[aModelIndex];
    gl_Position = aModel * vec4(aPos, 1.0);
}