#version 330 core
layout(points) in;
layout(line_strip, max_vertices = 16) out;

layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

in vec3 ext[];

void draw_vertex(vec3 v){
    vec3 bExt = ext[0];
    vec4 position = gl_in[0].gl_Position;
    gl_Position = u_projection * u_view * (position + vec4(bExt * v, 0.0));
    EmitVertex();
}

void main() {
    /* top */
    draw_vertex(vec3(1, 1, 1));
    draw_vertex(vec3(-1, 1, 1));
    draw_vertex(vec3(-1, 1, -1));
    draw_vertex(vec3(1, 1, -1));
    draw_vertex(vec3(1, 1, 1));

    /* right */
    draw_vertex(vec3(1, -1, 1));
    draw_vertex(vec3(1, -1, -1));
    draw_vertex(vec3(1, 1, -1));

    /* front */
    draw_vertex(vec3(1, -1, -1));
    draw_vertex(vec3(-1, -1, -1));
    draw_vertex(vec3(-1, 1, -1));

    /* left */
    draw_vertex(vec3(-1, -1, -1));
    draw_vertex(vec3(-1, -1, 1));
    draw_vertex(vec3(-1, 1, 1));

    /* back */ 
    draw_vertex(vec3(-1, -1, 1));
    draw_vertex(vec3(1, -1, 1));

    EndPrimitive();
}