#version 330 core

in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

out vec4 FragColor;

struct Light {
    vec4 position;
    vec3 diffuse;
    vec3 specular;
};

layout(std140) uniform Lights {
    vec3 u_light_ambient;
    Light u_lights[4];
};
// texture[0] - texture
// texture[1] - diffuse
// texture[2] - specular
uniform sampler2D u_texture[8];

layout(std140) uniform Material { float u_shiness; };

layout(std140) uniform Camera { vec3 u_cam_pos; };

vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 light_dir, vec3 view_dir,
                     float d) {
    vec3 n = normalize(normal);
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * vec3(texture(u_texture[1], texCoord)) * diff;
    vec3 specular_l = specular * vec3(texture(u_texture[2], texCoord)) * spec;
    // return specular_l;
    return att * (diffuse_l + specular_l);
}

void main() {
    vec3 light_out = vec3(texture(u_texture[1], texCoord)) * u_light_ambient;
    vec3 view_dir = normalize(u_cam_pos - fragPos);
    for (int i = 0; i < 4; i++) {
        Light light = u_lights[i];
        if (light.position.w == 0)
            continue;
        else if (light.position.w == -1) {
            vec3 light_dir = vec3(light.position) - fragPos;
            float d = length(light_dir);
            light_dir = normalize(light_dir);
            light_out += calculate_light(light.diffuse, light.specular,
                                         light_dir, view_dir, d);
        } else if (light.position.w == -2) {
            light_out +=
                calculate_light(light.diffuse, light.specular,
                                normalize(vec3(light.position)), view_dir,
                                1);
        }
    }

    FragColor = vec4(light_out, 1.0) * texture(u_texture[0], texCoord);
}