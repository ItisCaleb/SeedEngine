#version 410 core

in float height;
out vec4 FragColor;

void main() {
    float h = (height + 16)/64.0f;
    if(h < 0.01){
        discard;
    }
    FragColor = vec4(h, h, h, 1.0);
}