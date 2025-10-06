#version 330 core

in vec4 positionCS;
in vec4 positionVS;
out vec4 FragColor;

uniform sampler2D u_decal;


void main() {
    vec2 screen_pos = positionCS.xy / positionCS.w;
    vec2 depth_uv = screen_pos * vec2(0.5, -0.5) + vec2(0.5, 0.5);
    depth_uv += vec2(0.5, 0.5);
    vec4 depth = texture(u_decal, depth_uv);

    vec3 view_ray = positionVS.xyz * (1000 / -positionVS.z);
    vec3 view_pos = view_ray 
    FragColor = vec4(light_out, 1.0) * texture(u_diffuse, texCoord);
}