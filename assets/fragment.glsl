#version 330 core

in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

out vec4 FragColor;

struct PosLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct DirLight {
    vec3 dir;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(std140) uniform Lights {
    PosLight u_pos_light;
    DirLight u_dir_light;
};

layout(std140) uniform Material {
    vec3 u_ambient;
    vec3 u_diffuse;
    vec3 u_specular;
    float u_shiness;
};

layout (std140) uniform Camera
{
    vec3 u_cam_pos;
};

vec3 calculate_light(vec3 ambient, vec3 diffuse, vec3 specular, vec3 light_dir, vec3 view_dir, float d) {
    vec3 n = normalize(normal);
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_shiness);
    float att =  1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 ambient_l = ambient * u_ambient;
    vec3 diffuse_l = diffuse * u_diffuse * diff;
    vec3 specular_l = specular * u_specular * spec;
    //return specular_l;
    return ambient_l + att * (diffuse_l + specular_l);
}

void main() {
    vec3 light_out = vec3(0, 0, 0);
    vec3 light_dir = u_pos_light.position - fragPos;
    float d = length(light_dir);
    light_dir = normalize(light_dir);
    vec3 view_dir = normalize(u_cam_pos - fragPos);


    // light_out += calculate_light(u_dir_light.ambient, u_dir_light.diffuse,
    //                                  u_dir_light.specular, normalize(u_dir_light.dir), view_dir, d);

    light_out += calculate_light(u_pos_light.ambient, u_pos_light.diffuse,
                                     u_pos_light.specular, light_dir, view_dir, d);

    FragColor = vec4(light_out, 1.0);
}