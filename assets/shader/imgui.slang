import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 color;
    float4 sv_position : SV_Position;
}

struct Property{
    Sampler2D u_texture;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(vec2 aPos, vec2 aUV, vec4 aColor){
    VS_OUT out;
    out.sv_position = gProj * vec4(aPos.xy,0,1);
    out.uv = aUV;
    out.color = aColor;
    return out;
}


[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    return in.color * prop.u_texture.Sample(in.uv);
}