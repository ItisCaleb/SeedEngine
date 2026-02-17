import seed_inc;

struct VS_OUT{
    float2 uv;
    float4 sv_position : SV_Position;
}

struct Property{
    Sampler2D image;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(float2 aPos, float2 aTexCoord){
    VS_OUT out;
    out.sv_position = vec4(aPos, 0.0 , 1.0);
    out.uv = aTexCoord;
    return out;
}

[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    const float gamma = 1.0;
    vec3 hdrColor = prop.image.Sample(in.uv).rgb;
    // // exposure tone mapping
    // vec3 mapped =  hdrColor / (hdrColor + vec3(1.0));
    // // gamma correction
    // mapped = pow(mapped, vec3(1.0 / gamma));
    return vec4(hdrColor, 1.0);
}