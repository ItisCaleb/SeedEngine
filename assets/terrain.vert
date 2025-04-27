#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out vec2 texCoord;

void main(){
    gl_Position = u_projection * u_view * vec4(aPos, 1.0);
    texCoord = aTexCoord;
}