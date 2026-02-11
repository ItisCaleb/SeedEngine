import seed_inc;

[[vk_push_constant]]
ConstantBuffer<uint> split;

[shader("vertex")]
float4 vert(float2 aPos, uint aInstanceIndex): SV_Position{
    mat4 aModel = gTransform[aInstanceIndex];
    return gCSM.u_lightspaces[split] * aModel * vec4(aPos, 1.0);
}

