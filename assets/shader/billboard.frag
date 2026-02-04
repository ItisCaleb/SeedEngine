#version 450 core
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D u_billboard;

void main() { 
    vec4 color = texture(u_billboard, texCoord);
    if(color.a < 0.1)
        discard;
    FragColor = color;
}