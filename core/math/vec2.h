#ifndef _SEED_VEC2_H_
#define _SEED_VEC2_H_

#include <math.h>
#include "core/types.h"

namespace Seed {
struct Vec2 {
        union {
                struct {
                        f32 x, y;
                };
                f32 coord[2] = {0};
        };

        f32 &operator[](i32 axis) { return this->coord[axis]; }
        const f32 &operator[](i32 axis) const { return this->coord[axis]; }

        Vec2 operator+(const Vec2 &b) { return Vec2{x + b.x, y + b.y}; }

        void operator+=(const Vec2 &b) {
            x += b.x;
            y += b.y;
        }

        Vec2 operator-(const Vec2 &b) { return Vec2{x - b.x, y - b.y}; }

        Vec2 operator-() { return Vec2{-x, -y}; }

        void operator-=(const Vec2 &b) {
            x -= b.x;
            y -= b.y;
        }

        Vec2 operator*(const f32 s) { return Vec2{x * s, y * s}; }

        void operator*=(const f32 s) {
            x *= s;
            y *= s;
        }

        Vec2 operator/(const f32 s) { return Vec2{x / s, y / s}; }

        void operator/=(const f32 s) {
            x /= s;
            y /= s;
        }

        Vec2 operator*(const Vec2 &b) { return Vec2{x * b.x, y * b.y}; }

        bool operator==(const Vec2 &b) { return x == b.x && y == b.y; }

        f32 length() { return sqrtf(x * x + y * y); }

        f32 dot(const Vec2 &b) { return x * b.x + y * b.y; }

        Vec2 norm(const Vec2 &b) {
            f32 len = length();
            return Vec2{x / len, y / len};
        }

        Vec2 rotate(f32 rad) {
            f32 c = cosf(rad);
            f32 s = sinf(rad);
            return Vec2{c * x - s * y, s * x + c * y};
        }
};

}  // namespace Seed

#endif