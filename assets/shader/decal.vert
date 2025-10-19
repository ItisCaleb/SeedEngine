layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aModel;


layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out vec4 positionCS;
out vec4 positionVS;

void main(){
    positionVS = u_view * aModel * vec4(aPos, 1.0);
    positionCS = pos;
    gl_Position = positionCS;
}