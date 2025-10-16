#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in uint aModelIndex;


layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

layout(std430, binding = 0) buffer InstanceTransformDatas
{
    // 2^16
    mat4 b_models[65536];
};

out vec3 normal;
out vec2 texCoord;
out vec3 fragPos;

void main(){
    mat4 aModel = b_models[aModelIndex];
    gl_Position = u_projection * u_view * aModel * vec4(aPos, 1.0);
    normal = mat3(transpose(inverse(aModel))) * aNormal;
    texCoord = aTexCoord;
    fragPos = vec3(aModel * vec4(aPos, 1.0));
}