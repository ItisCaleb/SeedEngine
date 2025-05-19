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

uniform sampler2D u_diffuse;
uniform sampler2D u_specular;
uniform sampler2D u_normal;

layout(std140) uniform Material { float u_shiness; };

layout(std140) uniform Camera { vec3 u_cam_pos; };

vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 light_dir, vec3 view_dir,
                     float d, vec3 n) {
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * vec3(texture(u_diffuse, texCoord)) * diff;
    vec3 specular_l = specular * vec3(texture(u_specular, texCoord)) * spec;
    // return specular_l;
    return att * (diffuse_l + specular_l);
}

void main() {
    vec3 light_out = vec3(texture(u_diffuse, texCoord)) * u_light_ambient * 0.2;
    vec3 view_dir = normalize(u_cam_pos - fragPos);
    vec3 n_vec = normalize(normal);
    // vec3 n_vec = texture(u_normal, texCoord).rgb;
    // n_vec = normalize(n_vec * 2.0 - 1.0);   


    for (int i = 0; i < 4; i++) {
        Light light = u_lights[i];
        if (light.position.w == 0)
            continue;
        else if (light.position.w == -1) {
            vec3 light_dir = vec3(light.position) - fragPos;
            float d = length(light_dir);
            light_dir = normalize(light_dir);
            light_out += calculate_light(light.diffuse, light.specular,
                                         light_dir, view_dir, d, n_vec);
        } else if (light.position.w == -2) {
            light_out +=
                calculate_light(light.diffuse, light.specular,
                                normalize(vec3(light.position)), view_dir,
                                1,n_vec);
        }
    }

    FragColor = vec4(light_out, 1.0) * texture(u_diffuse, texCoord);
}