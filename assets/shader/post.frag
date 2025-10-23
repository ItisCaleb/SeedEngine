#version 450 core
out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D image;

void main()
{    
    const float gamma = 1.0;
    vec3 hdrColor = texture(image, texCoord).rgb;
    // exposure tone mapping
    vec3 mapped =  hdrColor / (hdrColor + vec3(1.0));
    // gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));
    FragColor = vec4(mapped, 1.0);
}