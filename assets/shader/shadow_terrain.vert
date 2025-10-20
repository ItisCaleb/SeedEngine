layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 8) in uint aInstanceIndex;


out vec2 texCoord;

void main(){
    vec4 terrain = b_terrain[aInstanceIndex];
    gl_Position = vec4(aPos.x + terrain.x, 0.0f, aPos.y + terrain.y, 1.0);
    texCoord = aTexCoord + terrain.zw;
}