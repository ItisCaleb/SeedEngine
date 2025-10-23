#version 450 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 256) out;

#include <shadow.glsl>

void main() {
    for (int i = 0;i<4;i++){
        gl_ViewportIndex = i;
        for (int j = 0; j < 3; ++j) {
            gl_Position = u_lightspaces[i] * gl_in[j].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }

    // for (int i = 0; i < 8; ++i) {
    //     gl_ViewportIndex = i + 1;
    //     for (int j = 0; j < 3; ++j) {
    //         gl_Position = u_position_lightspace[i] * gl_in[j].gl_Position;
    //         EmitVertex();
    //     }
    //     EndPrimitive();
    // }
}