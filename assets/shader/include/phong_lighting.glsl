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


vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 diffuse_sample, vec3 specular_sample, vec3 light_dir, vec3 view_dir,
                     float d, vec3 n) {
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * diffuse_sample * diff;
    vec3 specular_l = specular * specular_sample * spec;

    return att * (diffuse_l + specular_l);
}