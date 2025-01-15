#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_
#include "core/types.h"
#include "core/ref.h"
#include "core/math/vec2.h"
#include "core/math/mat3.h"
#include "core/math/mat4.h"
#include <string>

namespace Seed {
class Shader : public RefCounted {
   private:
    u32 id;

   public:
    Shader(u32 id);
    void use();
    u32 get_id();
    template <typename T>
    void uniform_apply(i32 location, T value);
    template <typename T>
    void uniform_apply(const std::string &name, T value) {
        uniform_apply<T>(uniform_location(name), value);
    }
    template <>
    void uniform_apply(i32 location, u32 value);
    template <>
    void uniform_apply(i32 location, i32 value);
    template <>
    void uniform_apply(i32 location, f32 value);
    template <>
    void uniform_apply(i32 location, f64 value);

    template <>
    void uniform_apply(i32 location, Vec2 value);
    template <>
    void uniform_apply(i32 location, Vec3 value);
    template <>
    void uniform_apply(i32 location, Vec4 value);
    template <>
    void uniform_apply(i32 location, Mat3 *value);
    template <>
    void uniform_apply(i32 location, Mat4 *value);

    int uniform_location(const std::string &name);
};

}  // namespace Seed

#endif