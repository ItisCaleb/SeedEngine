#ifndef _SEED_VEC2_H_
#define _SEED_VEC2_H_

#include <math.h>
#include <seed/types.h>

namespace Seed {
struct Vec2 {
    f32 x, y;

    inline Vec2 operator+(const Vec2 &b) { return Vec2{x + b.x, y + b.y}; }

    inline void operator+=(const Vec2 &b) {
        x += b.x;
        y += b.y;
    }

    inline Vec2 operator-(const Vec2 &b) { return Vec2{x - b.x, y - b.y}; }

    inline Vec2 operator-() { return Vec2{-x, -y}; }

    inline void operator-=(const Vec2 &b) {
        x -= b.x;
        y -= b.y;
    }

    inline Vec2 operator*(const f32 s) { return Vec2{x * s, y * s}; }

    inline void operator*=(const f32 s) {
        x *= s;
        y *= s;
    }

    inline Vec2 operator/(const f32 s) { return Vec2{x / s, y / s}; }

    inline void operator/=(const f32 s) {
        x /= s;
        y /= s;
    }

    inline f32 operator*(const Vec2 &b) { return x * b.x + y * b.y; }

    inline bool operator==(const Vec2 &b) { return x == b.x && y == b.y; }

    inline f32 length() { return sqrtf(x * x + y * y); }

    inline Vec2 norm(const Vec2 &b) {
        f32 len = length();
        return Vec2{x / len, y / len};
    }

    inline Vec2 rotate(f32 rad) {
        f32 c = cosf(rad);
        f32 s = sinf(rad);
        return Vec2{c * x - s * y, s * x + c * y};
    }
};

}  // namespace Seed

#endif