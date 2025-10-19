layout (quads, fractional_odd_spacing, ccw) in;

uniform sampler2D height_map;  // the texture corresponding to our height map
layout (std140) uniform Matrices
{
    mat4 u_projection;
    mat4 u_view;
};

layout (std140) uniform LightSpaceMatrices
{
    mat4 u_direction_lightspace;
    mat4 u_position_lightspace[8];
};

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out float height;
out vec3 normal;
out vec4 light_fragPos;
out vec4 fragPos;

float get_height(vec2 tex_coord){
    return texture(height_map, tex_coord).y * 256.0 - 128.0;
}

void main()
{
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates
    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;
    float offset = 0.0001;
    // lookup texel at patch coordinate for height and scale + shift as desired
    height = get_height(texCoord);
    float rh = get_height(texCoord + vec2(-offset, 0));
    float lh = get_height(texCoord + vec2(offset, 0));
    float uh = get_height(texCoord + vec2(0, offset));
    float dh = get_height(texCoord + vec2(0, -offset));

    // ----------------------------------------------------------------------
    // retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    // displace point along normal
    p += vec4(0, height, 0 , 0);
    
    //https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
    normal = normalize(vec3(2 * (rh - lh), -4, 2*(dh - uh)));
    fragPos = p;
    light_fragPos = u_direction_lightspace * p;
    // ----------------------------------------------------------------------
    // output patch point position in clip space
    gl_Position = u_projection * u_view * p;
}