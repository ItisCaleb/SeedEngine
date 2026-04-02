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

float3 ACESToneMapping(float3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

[shader("fragment")]
float4 frag(VS_OUT in) : SV_Target0{
    const float gamma = 2.2;
    // we flipped uv here
    in.uv.y = (1 - in.uv.y);
    vec3 hdrColor = prop.image.Sample(in.uv).rgb;
    // exposure tone mapping
    vec3 mapped =  ACESToneMapping(hdrColor, 0.5);
    // gamma correction
    // mapped = pow(mapped, vec3(1.0 / gamma));
    return vec4(mapped, 1.0);
}