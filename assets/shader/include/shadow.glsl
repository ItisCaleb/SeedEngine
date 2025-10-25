layout (std140) uniform LightSpaceMatrices
{
    mat4 u_lightspaces[64];
    vec4 u_shadow_uv[64];
    vec4 u_far;
};

float SHADOW_POS_OFF = 1.0;

vec3 shadow_pos_offset(sampler2D shadow_map, float n_dot_l, vec3 normal, float scale){
    float offset_scale = clamp(1.0 - n_dot_l, 0.0, 1.0);
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

float ShadowCalculation(sampler2D shadow_map, vec4 frag_pos, float frag_z, vec3 normal, vec3 light_dir)
{
    for(int i = 0;i < 4;i++){
        if(frag_z >= u_far[i]) continue; 
        vec3 n = normalize(normal);
        vec4 offset = vec4(shadow_pos_offset(shadow_map, dot(n, light_dir), n, i * 1.5), 0.0);
        vec4 lightspace_fragpos = u_lightspaces[i] * (frag_pos + offset);
        // perform perspective divide
        vec3 projCoords = lightspace_fragpos.xyz / lightspace_fragpos.w;
        // transform to [0,1] range
        projCoords = projCoords * 0.5 + 0.5;
        
        //it is outside on the sides
        if( projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z < 0.0 || projCoords.z > 1.0)
	    	continue;

        // get closest depth value from light’s perspective (using
        // [0,1] range fragPosLight as coords)
        // shadow map atlas transform
        projCoords.xy = projCoords.xy * u_shadow_uv[i].zw + u_shadow_uv[i].xy;

        vec2 tex_scale = (1.0 / vec2(textureSize(shadow_map, 0)));

        float bias = 0.005;

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
    return 0.0;
}