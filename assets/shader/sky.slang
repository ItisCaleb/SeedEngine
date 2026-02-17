import seed_inc;

struct VS_OUT{
    vec3 uv;
    vec4 sv_position : SV_Position;
}

struct Property{
    SamplerCube skybox;
};

ParameterBlock<Property> prop;

[shader("vertex")]
VS_OUT vert(vec3 aPos){
    VS_OUT output;
    output.uv = aPos;
    mat4 view = gMatrices.u_view;
    view[3].xyz = 0;
    vec4 pos = gMatrices.u_projection * view * vec4(aPos, 1.0);
    output.sv_position = pos.xyww;
    return output;
}


[shader("fragment")]
float4 frag(VS_OUT in){
    return prop.skybox.Sample(in.uv);
}