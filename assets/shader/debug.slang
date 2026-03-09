import seed_inc;

struct VS_OUT{
    float4 color;
    float4 sv_position : SV_Position;
}

[shader("vertex")]
VS_OUT vert(float2 aPos, float4 aColor){
    VS_OUT out;
    Camera cam = GetCamera();
    out.sv_position = cam.u_projection * cam.u_view * vec4(aPos, 1.0);
    out.color = aColor;
    return out;
}

[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    return in.color;
}