#include <seed/rendering/shader.h>
#include <glad/glad.h>


namespace Seed {
Shader::Shader(u32 id) : id(id) {};
void Shader::use() { glUseProgram(id); }
u32 Shader::get_id() { return id; }


template <>
void Shader::uniform_apply(i32 location, u32 value) {
    glUniform1ui(location, value);
}
template <>
void Shader::uniform_apply(i32 location, i32 value) {
    glUniform1i(location, value);
}
template <>
void Shader::uniform_apply(i32 location, f32 value) {
    glUniform1f(location, value);
}
template <>
void Shader::uniform_apply(i32 location, f64 value) {
    glUniform1d(location, value);
}
template <>
void Shader::uniform_apply(i32 location, Vec2 value) {
    glUniform2f(location, value.x, value.y);
}
template <>
void Shader::uniform_apply(i32 location, Vec3 value) {
    glUniform3f(location, value.x, value.y, value.z);
}
template <>
void Shader::uniform_apply(i32 location, Vec4 value) {
    glUniform4f(location, value.x, value.y, value.z, value.w);
}
template <>
void Shader::uniform_apply(i32 location, Mat3 *value) {
    glUniformMatrix3fv(location, 1, GL_TRUE, (const GLfloat *)value->data);
}
template <>
void Shader::uniform_apply(i32 location, Mat4 *value) {
    glUniformMatrix4fv(location, 1, GL_TRUE, (const GLfloat *)value->data);
}


int Shader::uniform_location(const std::string &name){
    return glGetUniformLocation(id, name.c_str());
}


}  // namespace Seed