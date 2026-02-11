import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position : SV_Position;
}

[shader("vertex")]
VS_OUT vert(float2 aPos, float2 aTexCoord, uint aInstanceIndex){
    VS_OUT output;
    float4x4 aModel = gTransform[aInstanceIndex];
    float3 scale = float3(aModel[0][0], aModel[1][1], aModel[2][2]);
    float3 pos = aModel[3].xyz;

    // calculate camera params
    float3 up = float3(0, 1, 0);
    float3 front = normalize(pos - gCam.u_cam_pos);
    float3 right = normalize(cross(front, up));

    float3 worldPos = pos + right * aPos.x * scale.x + up * aPos.y * scale.y;
    output.uv = aTexCoord;
    output.sv_position = mul(gMatrices.u_projection, gMatrices.u_view) * vec4(worldPos, 1.0);
    return output;
}


Sampler2D u_billboard;

[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    float4 color = u_billboard.Sample(in.uv);
    if(color.a < 0.1)
        discard;
    return color;
}