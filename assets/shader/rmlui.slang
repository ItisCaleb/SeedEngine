import seed_global;

struct RmlPushConstant{
    vec2 translation;
    vec2 padding;
    mat4 transform;
};

[[vk_push_constant]]
ConstantBuffer<RmlPushConstant> gRmlPC;

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
    vec4 pos = vec4(aPos.xy + gRmlPC.translation, 0, 1);
    out.sv_position = gProj * gRmlPC.transform * pos;
    out.uv = aUV;
    out.color = aColor;
    return out;
}

[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    return in.color * prop.u_texture.Sample(in.uv);
}