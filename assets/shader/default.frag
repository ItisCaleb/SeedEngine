in vec2 texCoord;
in vec3 fragPos;
in vec4 light_fragPos;
in vec3 view_pos;
in vec3 tangent_dir_light;
in vec3 tangent_pos_light[8];

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

uniform sampler2D u_diffuse;
uniform sampler2D u_specular;
uniform sampler2D u_normal;

uniform sampler2D shadowMap;

layout(std140) uniform Material { float u_shiness; };


float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 light_dir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light’s perspective (using
    // [0,1] range fragPosLight as coords)
    projCoords.x = projCoords.x * 0.5;
    projCoords.y = projCoords.y * 0.5 + 0.5;
    //it is outside on the sides
    if( projCoords.x < 0.0 || projCoords.x > 0.5 ||
        projCoords.y < 0.5 || projCoords.y > 1.0 )
		return 0.0;

    float bias = max(0.05 * (1.0- dot(normal, light_dir)), 0.005);

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light’s perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

vec3 calculate_light(vec3 diffuse, vec3 specular, vec3 light_dir, vec3 view_dir,
                     float d, vec3 n) {
    vec3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float att = 1 / d;
    float diff = max(dot(n, light_dir), 0.0);
    vec3 diffuse_l = diffuse * vec3(texture(u_diffuse, texCoord)) * diff;
    vec3 specular_l = specular * vec3(texture(u_specular, texCoord)) * spec;
    float shadow = ShadowCalculation(light_fragPos, n, light_dir);

    return att * (diffuse_l + specular_l) * (1.0 - shadow);;
}

void main() {
    vec3 light_out = vec3(texture(u_diffuse, texCoord)) * u_light_ambient;
    vec3 normal = texture(u_normal, texCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    vec3 view_dir = normalize(view_pos - fragPos);

    // direction
    light_out +=
        calculate_light(u_dir_light.diffuse, u_dir_light.specular,
                                normalize(tangent_dir_light), view_dir,
                                1,normal);

    for (int i = 0; i < 8; i++) {
        Light light = u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = tangent_pos_light[i] - fragPos;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
                                         light_dir, view_dir, d, normal);
    }
    

    FragColor = vec4(light_out, 1.0);
}