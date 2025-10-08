#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main(){
    gl_Position = vec4(aPos.x, 0.0f, aPos.y, 1.0);
    texCoord = aTexCoord;
}