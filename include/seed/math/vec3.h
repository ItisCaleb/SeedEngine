#ifndef _SEED_VEC3_H_
#define _SEED_VEC3_H_

#include <math.h>
#include <seed/types.h>


namespace Seed {
struct Vec3 {
    f32 x, y, z;

    inline Vec3 operator+(const Vec3 &b) {
        return Vec3{x + b.x, y + b.y, z + b.z};
    }

    inline void operator+=(const Vec3 &b) {
        x += b.x;
        y += b.y;
        z += b.z;
    }

    inline Vec3 operator-(const Vec3 &b) {
        return Vec3{x - b.x, y - b.y, z - b.z};
    }

    inline Vec3 operator-() { return Vec3{-x, -y, -z}; }

    inline void operator-=(const Vec3 &b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
    }

    inline Vec3 operator*(const f32 s) { return Vec3{x * s, y * s, z * s}; }

    inline void operator*=(const f32 s) {
        x *= s;
        y *= s;
        z *= s;
    }

    inline Vec3 operator/(const f32 s) { return Vec3{x / s, y / s, z / s}; }

    inline void operator/=(const f32 s) {
        x /= s;
        y /= s;
        z /= s;
    }

    inline f32 operator*(const Vec3 &b) { return x * b.x + y * b.y + z * b.z; }

    inline bool operator==(const Vec3 &b) {
        return x == b.x && y == b.y && z == b.z;
    }

    inline Vec3 cross(const Vec3 &b) {
        return Vec3{y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }

    inline f32 length() { return sqrtf(x * x + y * y + z * z); }

    inline Vec3 norm(const Vec3 &b) {
        f32 len = length();
        return Vec3{x / len, y / len, z / len};
    }

    inline Vec3 rotateX(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{x, c * y - s * z, s * y + c * z};
    }

    inline Vec3 rotateY(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{c * x + s * z, y, -s * x + c * z};
    }

    inline Vec3 rotateZ(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec3{c * x - s * y, s * x + c * y, z};
    }
};

}  // namespace Seed

#endif