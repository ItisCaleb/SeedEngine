#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aModel;


void main(){
    gl_Position = aModel * vec4(aPos, 1.0);
}