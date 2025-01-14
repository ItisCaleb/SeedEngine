#ifndef _SEED_VEC3_H_
#define _SEED_VEC3_H_

#include <math.h>
#include <seed/types.h>

namespace Seed {
struct Vec3 {
    union {
        struct {
            f32 x, y, z;
        };
        f32 coord[3] = {0};
    };

    f32 &operator[](i32 axis) { return this->coord[axis]; }

    const f32 &operator[](i32 axis) const { return this->coord[axis]; }

    Vec3 operator+(const Vec3 &b) { return Vec3{x + b.x, y + b.y, z + b.z}; }

    void operator+=(const Vec3 &b) {
        x += b.x;
        y += b.y;
        z += b.z;
    }

    Vec3 operator-(const Vec3 &b) { return Vec3{x - b.x, y - b.y, z - b.z}; }

    Vec3 operator-() { return Vec3{-x, -y, -z}; }

    void operator-=(const Vec3 &b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
    }

    Vec3 operator*(const f32 s) { return Vec3{x * s, y * s, z * s}; }

    void operator*=(const f32 s) {
        x *= s;
        y *= s;
        z *= s;
    }

    Vec3 operator/(const f32 s) { return Vec3{x / s, y / s, z / s}; }

    void operator/=(const f32 s) {
        x /= s;
        y /= s;
        z /= s;
    }

    Vec3 operator*(const Vec3 &b) { return Vec3{x * b.x, y * b.y, z * b.z}; }

    bool operator==(const Vec3 &b) { return x == b.x && y == b.y && z == b.z; }

    Vec3 cross(const Vec3 &b) {
        return Vec3{y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }

    f32 length() { return sqrtf(x * x + y * y + z * z); }

    Vec3 norm() {
        f32 len = length();
        return Vec3{x / len, y / len, z / len};
    }

    f32 dot(const Vec3 &b) { return x * b.x + y * b.y + z * b.z; }

    Vec3 rotateX(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{x, c * y - s * z, s * y + c * z};
    }

    Vec3 rotateY(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{c * x + s * z, y, -s * x + c * z};
    }

    Vec3 rotateZ(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{c * x - s * y, s * x + c * y, z};
    }
};

}  // namespace Seed

#endif