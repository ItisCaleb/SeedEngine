import seed_inc;


struct VS_OUT{
    vec2 texCoord;
    vec3 frag_pos;
    vec3 normal;
    vec3 tangent_frag_pos;
    vec3 tangent_view_pos;
    float view_depth;
    vec3 tangent_dir_light;
    vec3 tangent_pos_light[8];
    vec4 sv_position : SV_Position;
}

struct Property{
    Sampler2D u_diffuse;
    Sampler2D u_specular;
    Sampler2D u_normal;
    Sampler2DShadow shadowMap;
};

ParameterBlock<Property> prop;

struct VS_IN{
    vec3 aPos;
    vec3 aNormal;
    vec3 aTangent;
    vec2 aUV;
#ifdef BONE
    uint4 aBoneIds;
    vec4 aWeights;
#endif
    uint id: SV_VulkanInstanceID;
};

[shader("vertex")]
VS_OUT vert(VS_IN in){
    VS_OUT out;
    Camera cam = GetCamera();

#ifdef BONE
    mat4 aModel = GetSkeletonTransform(in.id);
#else
    mat4 aModel = GetTransform(in.id);
#endif
    vec4 position = vec4(in.aPos, 1.0);
    vec3 normal = in.aNormal;
    vec3 tangent = in.aTangent;
#ifdef BONE
    vec4 totalPos = vec4(0.0f);
    vec3 skinnedNormal = vec3(0.0f);
    vec3 skinnedTangent = vec3(0.0f);
    for(int i = 0;i<4;i++){
        if(in.aWeights[i] == 0.0 || in.aBoneIds[i] == -1) continue;
        mat4 bone_mat = GetSkeletonBone(in.id, in.aBoneIds[i]);
        totalPos += bone_mat * position * in.aWeights[i];
        mat3 normal_mat = mat3(bone_mat);
        skinnedNormal += normal_mat * normal * in.aWeights[i];
        skinnedTangent += normal_mat * tangent * in.aWeights[i];
    }
    position = totalPos;
    normal = skinnedNormal;
    tangent = skinnedTangent;
#endif

    vec4 worldPos = aModel * position;

    // For right handed system,
    // after view transformation, the depth is always negative
    // So we negate it
    out.view_depth = -(cam.u_view * worldPos).z;
    out.sv_position = cam.u_projection * cam.u_view * worldPos;


    // tangent space transition
    mat3 TBN_to_world = mat3(1.0);
    if(length(normal) > 0.01){
        mat3 modelNormalMat = mat3(aModel);

        vec3 T = normalize(modelNormalMat * tangent);
        vec3 N = normalize(modelNormalMat * normal);
        T = normalize(T - dot(T, N) * N);  // Gram-Schmidt
        vec3 B = cross(N, T);
        TBN_to_world = mat3(T, B, N);
    }

    mat3 TBN_to_tangent = transpose(TBN_to_world);

    out.texCoord = in.aUV;

    out.frag_pos = worldPos.xyz;
    out.normal = normal;
    out.tangent_frag_pos = TBN_to_tangent * worldPos.xyz;
    out.tangent_view_pos = TBN_to_tangent * cam.u_cam_pos;
    out.tangent_dir_light = TBN_to_tangent * (gLights.u_dir_light.position);

    for (int i = 0; i < 8; i++) {
        Light light = gLights.u_point_lights[i];
        if (light.enable == 0)
            continue;
        out.tangent_pos_light[i] = TBN_to_tangent * (light.position); 
    }
    return out;
}



[shader("fragment")]
float4 frag(VS_OUT in){
    vec3 normal = prop.u_normal.Sample(in.texCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    vec3 view_dir = normalize(in.tangent_view_pos - in.tangent_frag_pos);
    vec3 diffuse_sample = prop.u_diffuse.Sample(in.texCoord).xyz;
    vec3 specular_sample = prop.u_specular.Sample(in.texCoord).xyz;
    vec3 dir_light = -normalize(in.tangent_dir_light);

    vec3 light_out = diffuse_sample * gLights.u_light_ambient;

    // direction
    float dir_shadow = (1.0 - CSMShadowCalculation(prop.shadowMap,
         vec4(in.frag_pos, 1.0), in.view_depth, in.normal, dir_light));
    light_out +=
        calculate_light(gLights.u_dir_light.diffuse, gLights.u_dir_light.specular,
                         diffuse_sample, specular_sample,
                                dir_light, view_dir,
                                1, normal) * dir_shadow;


    for (int i = 0; i < 8; i++) {
        Light light = gLights.u_point_lights[i];
        if (light.enable == 0)
            continue;
        vec3 light_dir = in.tangent_pos_light[i] - in.frag_pos;
        float d = length(light_dir);
        light_dir = normalize(light_dir);
        light_out += calculate_light(light.diffuse, light.specular,
            diffuse_sample, specular_sample,
            light_dir, view_dir, d, normal);
    }
    

    return vec4(light_out, 1.0);
}