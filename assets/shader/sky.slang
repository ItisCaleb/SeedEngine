import seed_inc;

struct VS_OUT{
    vec3 uv;
    vec4 sv_position : SV_Position;
}

[shader("vertex")]
VS_OUT vert(vec3 aPos){
    VS_OUT output;
    output.uv = aPos;
    vec4 pos = mul(gMatrices.u_projection, gMatrices.u_view) * vec4(aPos, 1.0);
    output.sv_position = pos.xyww;
    return output;
}


SamplerCube skybox;

[shader("fragment")]
float4 frag(VS_OUT in){
    return skybox.Sample(in.uv);
}