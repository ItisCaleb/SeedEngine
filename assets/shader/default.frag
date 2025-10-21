#version 450 core
in vec2 texCoord;
in vec3 fragPos;
in vec4 light_fragPos;
in vec3 view_pos;
in vec3 tangent_dir_light;
in vec3 tangent_pos_light[8];

out vec4 FragColor;

#include <phong_lighting.glsl>

layout(std140) uniform Camera { vec3 u_cam_pos; };

uniform sampler2D u_diffuse;
uniform sampler2D u_specular;
uniform sampler2D u_normal;

uniform sampler2D shadowMap;

layout(std140) uniform Material { float u_shiness; };


void main() {
    vec3 light_out = vec3(texture(u_diffuse, texCoord)) * u_light_ambient;
    vec3 normal = texture(u_normal, texCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    vec3 view_dir = normalize(view_pos - fragPos);
    vec3 diffuse_sample = vec3(texture(u_diffuse, texCoord));
    vec3 specular_sample = vec3(texture(u_specular, texCoord));

    // direction
    light_out +=
        calculate_light(u_dir_light.diffuse, u_dir_light.specular,
                         diffuse_sample, specular_sample,
                                normalize(tangent_dir_light), view_dir,
                                1, normal, shadowMap, light_fragPos);

    for (int i = 0; i < 8; i++) {
        Light light = u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = tangent_pos_light[i] - fragPos;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
            diffuse_sample, specular_sample,
            light_dir, view_dir, d, normal, shadowMap, light_fragPos);
    }
    

    FragColor = vec4(light_out, 1.0);
}