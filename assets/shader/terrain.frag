#version 450 core
in float height;
in vec3 normal;
in vec4 fragPos;
out vec4 FragColor;

#include <shadow.glsl>
#include <phong_lighting.glsl>


layout(std140) uniform Camera { vec3 u_cam_pos; };

uniform sampler2D shadowMap;

void main() {
    float h = (height + 128)/256.0f;
    if(h < 0.01){
        discard;
    }

    vec3 light_out = u_light_ambient  * vec3(h, h, h);
    vec3 view_dir = normalize(u_cam_pos - fragPos.xyz);
    vec3 diffuse_sample = vec3(1,1,1);
    vec3 specular_sample = vec3(1,1,1);
    // direction
    light_out +=
        calculate_light(u_dir_light.diffuse, u_dir_light.specular,
            diffuse_sample, specular_sample,
            normalize(vec3(u_dir_light.position)), view_dir,
            1, normal) * (1.0 - ShadowCalculation(shadowMap, fragPos, normal, normalize(vec3(u_dir_light.position))));

    for (int i = 0; i < 8; i++) {
        Light light = u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = vec3(light.position) - fragPos.xyz;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
            diffuse_sample, specular_sample,
            light_dir, view_dir, d, normal);
    }


    FragColor = vec4(light_out, 1.0);
}