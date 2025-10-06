#version 410 core

in float height;
in vec3 normal;
in vec3 fragPos;
out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    float enable;
};

layout(std140) uniform Lights {
    vec3 u_light_ambient;
    Light u_dir_light;
    Light u_point_lights[8];
};

layout(std140) uniform Camera { vec3 u_cam_pos; };

vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 light_dir, vec3 view_dir,
                     float d, vec3 n) {
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * diff;
    vec3 specular_l = specular * spec;
    // return specular_l;
    return att * (diffuse_l + specular_l);
}

void main() {
    float h = (height + 16)/64.0f;
    if(h < 0.01){
        discard;
    }
    vec3 light_out = u_light_ambient * 0.2;
    vec3 view_dir = normalize(u_cam_pos - fragPos);
    vec3 n_vec = normal;

    // direction
    light_out +=
        calculate_light(u_dir_light.diffuse, u_dir_light.specular,
                                normalize(vec3(u_dir_light.position)), view_dir,
                                1,n_vec);

    for (int i = 0; i < 8; i++) {
        Light light = u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = vec3(light.position) - fragPos;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
                                         light_dir, view_dir, d, n_vec);
    }


    FragColor = vec4(light_out, 1.0);
}