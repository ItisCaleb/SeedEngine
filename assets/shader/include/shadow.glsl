layout (std140) uniform CSMShadow
{
    mat4 u_lightspaces[4];
    vec4 u_shadow_uv[4];
    vec4 u_far;
    vec4 u_shadow_unit;
};


float SHADOW_POS_OFF = 1.0;

vec3 shadow_pos_offset(sampler2D shadow_map, float n_dot_l, vec3 normal, float scale){
    float offset_scale = clamp(1 - n_dot_l, 0.0, 1.0);
    return offset_scale * scale * normal;
}

float shadow_bilinear(sampler2D shadow_map, vec2 uv, vec2 tex_scale, float sample_depth){
    float r = texture(shadow_map, uv + vec2(1,0) * tex_scale).r;
    float l = texture(shadow_map, uv + vec2(-1,0) * tex_scale).r;
    float t = texture(shadow_map, uv + vec2(0,1) * tex_scale).r;
    float b = texture(shadow_map, uv + vec2(0,-1) * tex_scale).r;
    r = sample_depth > r ? 1.0 : 0.0;
    l = sample_depth > l ? 1.0 : 0.0;
    t = sample_depth > t ? 1.0 : 0.0;
    b = sample_depth > b ? 1.0 : 0.0;
    return (l + r + t + b) * 0.25;
}

vec2 recieve_plane_bias_uv(vec3 projCoords){
    vec3 pc_dx = dFdxFine(projCoords);
    vec3 pc_dy = dFdyFine(projCoords);
    vec2 depth_duv;
    depth_duv.x = pc_dy.y * pc_dx.z - pc_dx.y * pc_dy.z;
    depth_duv.y = pc_dx.x * pc_dy.z - pc_dy.x * pc_dx.z;
    float inv_det = 1.0 / ((pc_dx.x * pc_dy.y) - (pc_dx.y * pc_dy.x));
    depth_duv *= inv_det;
    return depth_duv;
}

int SelectCasacade(float depth){
    int casacade = 3;
    for(int i = 3;i >= 0;i--){
        if(depth < u_far[i]) casacade = i;
    }
    return casacade;
}

float ShadowCalculation(sampler2D shadow_map, vec4 frag_pos, float frag_z, vec3 normal, vec3 light_dir)
{
    int casacade_idx = SelectCasacade(frag_z);
    if(casacade_idx == 4) return 0.0;
    vec3 n = normalize(normal);
    vec4 offset = u_shadow_unit[casacade_idx] * vec4(shadow_pos_offset(shadow_map, dot(n, light_dir), n, 2), 0.0);
    vec4 lightspace_fragpos = u_lightspaces[casacade_idx] * (frag_pos + offset);
    // perform perspective divide
    vec3 projCoords = lightspace_fragpos.xyz / lightspace_fragpos.w;
    vec2 depth_duv = recieve_plane_bias_uv(projCoords);
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // get closest depth value from light’s perspective (using
    // [0,1] range fragPosLight as coords)
    // shadow map atlas transform
    projCoords.xy = projCoords.xy * u_shadow_uv[casacade_idx].zw + u_shadow_uv[casacade_idx].xy;
    vec2 tex_scale = (1.0 / vec2(textureSize(shadow_map, 0)));
    vec4 normal_bias = vec4(0.02, 0.02, 0.03, 0.05);
    float bias = normal_bias[casacade_idx] * dot(normal, light_dir);
    float shadow = 0;
    float currentDepth = projCoords.z - bias;
    float pcf_count = 3.0;
    float o = (pcf_count - 1) / 2;
    float x, y;
    for(y = -o; y <= o; y += 1.0){
        for (x = -o;x <= o; x += 1.0){
            vec2 sample_offset = vec2(x, y) * tex_scale;
            float real_depth = currentDepth;
            shadow += shadow_bilinear(shadow_map,
                projCoords.xy + sample_offset, tex_scale, real_depth);
            // float _sample = texture(shadow_map, projCoords.xy + sample_offset).r;
            // shadow += real_depth > _sample ? 1.0 : 0.0;
        }
    }
    shadow /= (pcf_count) * (pcf_count);
    return shadow;
}