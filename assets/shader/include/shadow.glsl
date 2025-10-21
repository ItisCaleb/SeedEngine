float ShadowCalculation(sampler2D shadow_map, vec4 fragPosLightSpace, vec3 normal, vec3 light_dir)
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

    float closestDepth = texture(shadow_map, projCoords.xy).r;
    // get depth of current fragment from light’s perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}