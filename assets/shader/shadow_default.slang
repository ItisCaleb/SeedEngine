import seed_inc;

[[vk_push_constant]]
ConstantBuffer<uint> split;

[shader("vertex")]
float4 vert(float2 aPos, uint id: SV_VulkanInstanceID): SV_Position{
    mat4 aModel = gTransform[gVisible[id]];
    return gCSM.u_lightspaces[split] * aModel * vec4(aPos, 1.0);
}

