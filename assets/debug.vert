#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aExt;

out vec3 ext;

void main(){
    gl_Position = vec4(aPos, 1.0);
    ext = aExt;
}