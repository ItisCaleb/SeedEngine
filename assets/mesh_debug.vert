#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aModel;


layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

void main(){
    gl_Position = u_projection * u_view * aModel * vec4(aPos, 1.0);
}