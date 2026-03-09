import seed_inc;


struct VS_OUT{
    vec2 texCoord;
    vec3 fragPos;
    vec3 view_pos;
    float view_depth;
    vec3 tangent_dir_light;
    // vec3 tangent_pos_light[8];
    vec4 sv_position : SV_Position;
}

struct Property{
    Sampler2D u_diffuse;
    Sampler2D u_specular;
    Sampler2D u_normal;
    Sampler2D shadowMap;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(vec3 aPos, vec3 aNormal, vec3 aTangent, vec2 aTexCoord, uint id: SV_VulkanInstanceID){
    VS_OUT out;
    mat4 aModel = GetTransform(id);
    vec4 worldPos = aModel * vec4(aPos, 1.0);
    // For right handed system,
    // after view transformation, the depth is always negative
    // So we negate it
    Camera cam = GetCamera();
    out.view_depth = -(cam.u_view * worldPos).z;
    out.sv_position = cam.u_projection * cam.u_view * worldPos;

    mat3 TBN_to_world = mat3(1.0);
    if(length(aNormal) > 0.01){
        vec3 T = normalize((aModel * vec4(aTangent.xyz, 0.0)).xyz);
        vec3 N = normalize((aModel * vec4(aNormal, 0.0)).xyz);
        T = normalize(T - dot(T, N) * N);  // Gram-Schmidt
        vec3 B = cross(N, T);
        TBN_to_world = mat3(T, B, N);
    }

    mat3 TBN_to_tangent = transpose(TBN_to_world);

    out.texCoord = aTexCoord;

    out.fragPos = TBN_to_tangent * worldPos.xyz;
    out.view_pos = TBN_to_tangent * cam.u_cam_pos;
    out.tangent_dir_light = TBN_to_tangent * (gLights.u_dir_light.position);

    // for (int i = 0; i < 8; i++) {
    //     Light light = gLights.u_point_lights[i];
    //     if (light.enable == 0)
    //         continue;
    //     out.tangent_pos_light[i] = TBN_to_tangent * (light.position); 
    // }
    return out;
}



[shader("fragment")]
float4 frag(VS_OUT in){
    vec3 light_out = prop.u_diffuse.Sample(in.texCoord).rgb * gLights.u_light_ambient;
    vec3 normal = prop.u_normal.Sample(in.texCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    vec3 view_dir = normalize(in.view_pos - in.fragPos);
    vec3 diffuse_sample = prop.u_diffuse.Sample(in.texCoord).xyz;
    vec3 specular_sample = prop.u_specular.Sample(in.texCoord).xyz;
    vec3 dir_light = -normalize(in.tangent_dir_light);
    // direction
    light_out +=
        calculate_light(gLights.u_dir_light.diffuse, gLights.u_dir_light.specular,
                         diffuse_sample, specular_sample,
                                dir_light, view_dir,
                                1, normal) *
                                (1.0 - ShadowCalculation(prop.shadowMap, vec4(in.fragPos, 1.0), in.view_depth, normal, dir_light));

    // for (int i = 0; i < 8; i++) {
    //     Light light = gLights.u_point_lights[i];
    //     if (light.enable == 0)
    //         continue;
    //     vec3 light_dir = in.tangent_pos_light[i] - in.fragPos;
    //     float d = length(light_dir);
    //     light_dir = normalize(light_dir);
    //     light_out += calculate_light(light.diffuse, light.specular,
    //         diffuse_sample, specular_sample,
    //         light_dir, view_dir, d, normal);
    // }
    

    return vec4(light_out, 1.0);
}