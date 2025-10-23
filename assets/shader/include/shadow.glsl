layout (std140) uniform LightSpaceMatrices
{
    mat4 u_lightspaces[64];
    vec4 u_shadow_uv[64];
};

float ShadowCalculation(sampler2D shadow_map, vec4 frag_pos, vec3 normal, vec3 light_dir)
{
    for(int i = 0;i < 3;i++){
        vec4 lightspace_fragpos = u_lightspaces[i] * frag_pos;
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
        projCoords.x = projCoords.x * u_shadow_uv[i].z + u_shadow_uv[i].x;
        projCoords.y = projCoords.y * u_shadow_uv[i].w + u_shadow_uv[i].y;

        float bias = max(0.05 * (1.0- dot(normal, light_dir)), 0.005);

        float shadow = 0;
        float x, y;
        vec2 tex_scale = (1 / (textureSize(shadow_map, 0) * 0.5));
        float currentDepth = projCoords.z;
        for(y = -1.5; y <= 1.5; y += 1.0){
            for (x = -1.5;x <= 1.5; x += 1.0){
               float _sample = texture(shadow_map, projCoords.xy + vec2(x,y) * tex_scale).r;
               shadow += currentDepth - bias > _sample ? 1.0 : 0.0;
            }
        }
        shadow /= 16.0;
        return shadow;
    }
    return 0.0;
}