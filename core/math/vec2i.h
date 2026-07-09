#ifndef _SEED_VEC2I_H_
#define _SEED_VEC2I_H_

#include <math.h>
#include "core/types.h"
#include "core/math/utils.h"

namespace Seed {
struct Vec2i {
        union {
                struct {
                        i32 x, y;
                };
                i32 coord[2] = {0};
        };

        i32 &operator[](i32 axis) { return this->coord[axis]; }
        const i32 &operator[](i32 axis) const { return this->coord[axis]; }

        Vec2i operator+(const Vec2i &b) const {
            return Vec2i{x + b.x, y + b.y};
        }

        void operator+=(const Vec2i &b) {
            x += b.x;
            y += b.y;
        }

        Vec2i operator-(const Vec2i &b) const {
            return Vec2i{x - b.x, y - b.y};
        }

        Vec2i operator-() const { return Vec2i{-x, -y}; }

        void operator-=(const Vec2i &b) {
            x -= b.x;
            y -= b.y;
        }

        Vec2i operator*(const i32 s) const { return Vec2i{x * s, y * s}; }

        void operator*=(const i32 s) {
            x *= s;
            y *= s;
        }

        Vec2i operator/(const i32 s) const { return Vec2i{x / s, y / s}; }

        void operator/=(const i32 s) {
            x /= s;
            y /= s;
        }

        Vec2i operator*(const Vec2i &b) const {
            return Vec2i{x * b.x, y * b.y};
        }

        bool operator==(const Vec2i &b) { return x == b.x && y == b.y; }

        f32 length() const { return sqrtf(x * x + y * y); }

        f32 length_sqr() const { return x * x + y * y; }

        f32 dot(const Vec2i &b) const { return x * b.x + y * b.y; }

        Vec2i norm(const Vec2i &b) const {
            i32 len = length();
            return Vec2i{x / len, y / len};
        }

        Vec2i rotate(f32 rad) const {
            f32 c = cosf(rad);
            f32 s = sinf(rad);
            return Vec2i{(i32)(c * x - s * y), (i32)(s * x + c * y)};
        }

        static Vec2i lerp(const Vec2i &a, const Vec2i &b, f32 t) {
            return a + (b - a) * clampf(t, 0.0, 1.0);
        }
};

}  // namespace Seed

#endif