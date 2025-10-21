#version 450 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140) uniform LightSpaceMatrices
{
    mat4 u_direction_lightspace;
    mat4 u_position_lightspace[8];
};


void main() {
    gl_ViewportIndex = 0;
    for (int j = 0; j < 3; ++j) {
        gl_Position = u_direction_lightspace * gl_in[j].gl_Position;
        EmitVertex();
    }
    EndPrimitive();

    // for (int i = 0; i < 8; ++i) {
    //     gl_ViewportIndex = i + 1;
    //     for (int j = 0; j < 3; ++j) {
    //         gl_Position = u_position_lightspace[i] * gl_in[j].gl_Position;
    //         EmitVertex();
    //     }
    //     EndPrimitive();
    // }
}