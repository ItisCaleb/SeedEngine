#version 450 core
in float height;
in vec3 normal;
in vec4 fragPos;
in float view_depth;
in vec2 texCoord;
out vec4 FragColor;

#include <shadow.glsl>
#include <phong_lighting.glsl>


layout(std140) uniform Camera { vec3 u_cam_pos; };

uniform sampler2D terrain_shadowMap;

uniform sampler2D shadowMap;

float terrain_shadowmapping(vec2 uv, float angle){
    float shadow = 0;
    float offset = 0.0001;
    // lookup texel at patch coordinate for height and scale + shift as desired
    float slope = texture(terrain_shadowMap, uv).r;
    float rh = texture(terrain_shadowMap, uv + vec2(offset, 0)).r;
    float lh = texture(terrain_shadowMap, uv + vec2(-offset, 0)).r;
    float uh = texture(terrain_shadowMap, uv + vec2(0, offset)).r;
    float dh = texture(terrain_shadowMap, uv + vec2(0, -offset)).r;
    shadow += angle > slope ? 1.0 : 0.0;
    shadow += angle > rh ? 1.0 : 0.0;
    shadow += angle > lh ? 1.0 : 0.0;
    shadow += angle > uh ? 1.0 : 0.0;
    shadow += angle > dh ? 1.0 : 0.0;

    return shadow / 5;
}

void main() {
    float h = (height + 128)/256.0f;
    if(h < 0.01){
        discard;
    }

    vec3 light_out = u_light_ambient  * vec3(h, h, h);
    vec3 view_dir = normalize(u_cam_pos - fragPos.xyz);
    vec3 diffuse_sample = vec3(1,1,1);
    vec3 specular_sample = vec3(1,1,1);
    vec3 dir_light = -normalize(vec3(u_dir_light.position));
    float c = dir_light.y;
    // direction
    light_out +=
        calculate_light(u_dir_light.diffuse, u_dir_light.specular,
            diffuse_sample, specular_sample, dir_light, view_dir,
            1, normal) * (1.0 - ShadowCalculation(shadowMap, fragPos, view_depth, normal, dir_light))
    * (1.0 - terrain_shadowmapping(texCoord, c));
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
    float dist = length(u_cam_pos - fragPos.xyz);
    float fog_factor = clamp((dist - 700) / 100, 0.0, 0.95);
    vec3 final_color = mix(light_out, vec3(0.48, 0.80, 0.80), fog_factor);

    FragColor = vec4(final_color, 1.0);
}