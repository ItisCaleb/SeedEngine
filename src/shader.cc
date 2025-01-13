#include <seed/rendering/shader.h>
#include <glad/glad.h>

namespace Seed{
    Shader::Shader(u32 id):id(id){};
    void Shader::use(){
        glUseProgram(id);
    }
}