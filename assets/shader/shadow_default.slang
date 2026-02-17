import seed_inc;


[shader("vertex")]
float4 vert(vec3 aPos, uint id: SV_VulkanInstanceID): SV_Position{
    mat4 aModel = GetTransform(id);
    mat4 aLSpace = GetLightSpace();
    return aLSpace * aModel * vec4(aPos, 1.0);
}

