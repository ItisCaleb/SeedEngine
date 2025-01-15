#ifndef _SEED_VEC4_H_
#define _SEED_VEC4_H_

#include <math.h>
#include "core/types.h"

namespace Seed {
struct Vec4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        f32 coord[4] = {0};
    };

    f32 &operator[](i32 axis) { return this->coord[axis]; }

    const f32 &operator[](i32 axis) const { return this->coord[axis]; }

    Vec4 operator+(const Vec4 &b) {
        return Vec4{x + b.x, y + b.y, z + b.z, w + b.w};
    }

    void operator+=(const Vec4 &b) {
        x += b.x;
        y += b.y;
        z += b.z;
        w += b.w;
    }

    Vec4 operator-(const Vec4 &b) {
        return Vec4{x - b.x, y - b.y, z - b.z, w - b.w};
    }

    Vec4 operator-() { return Vec4{-x, -y, -z, -w}; }

    void operator-=(const Vec4 &b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        w -= b.w;
    }

    Vec4 operator*(const f32 s) { return Vec4{x * s, y * s, z * s, w * s}; }

    void operator*=(const f32 s) {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
    }

    Vec4 operator/(const f32 s) { return Vec4{x / s, y / s, z / s, w / s}; }

    void operator/=(const f32 s) {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
    }

    Vec4 operator*(const Vec4 &b) {
        return Vec4{x * b.x, y * b.y, z * b.z, w * b.w};
    }

    bool operator==(const Vec4 &b) {
        return x == b.x && y == b.y && z == b.z && w == b.w;
    }

    Vec4 cross(const Vec4 &b) {
        return Vec4{y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }

    f32 length() { return sqrtf(x * x + y * y + z * z + w * w); }

    Vec4 norm(const Vec4 &b) {
        f32 len = length();
        return Vec4{x / len, y / len, z / len, w / len};
    }

    f32 dot(const Vec4 &b) { return x * b.x + y * b.y + z * b.z + w * b.w; }

};

}  // namespace Seed

#endif