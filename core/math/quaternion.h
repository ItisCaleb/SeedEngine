#ifndef _SEED_QUATERNION_H_
#define _SEED_QUATERNION_H_
#include "core/types.h"
#include "utils.h"
#include "vec3.h"
#include <math.h>

namespace Seed {

/*
    Stored as [w, x, y, z]
*/
struct Quaternion {
        f32 w, x, y, z;

        Quaternion operator+(const Quaternion &b) const {
            return Quaternion{w + b.w, x + b.x, y + b.y, z + b.z};
        }

        void operator+=(const Quaternion &b) {
            w += b.w;
            x += b.x;
            y += b.y;
            z += b.z;
        }

        Quaternion operator-(const Quaternion &b) const {
            return Quaternion{w - b.w, x - b.x, y - b.y, z - b.z};
        }

        void operator-=(const Quaternion &b) {
            w -= b.w;
            x -= b.x;
            y -= b.y;
            z -= b.z;
        }

        Quaternion operator*(const Quaternion &b) const {
            return Quaternion{w * b.w - x * b.x - y * b.y - z * b.z,
                              w * b.x + x * b.w + y * b.z - z * b.y,
                              w * b.y + x * b.z + y * b.w + z * b.x,
                              w * b.z + x * b.y - y * b.x + z * b.w};
        }

        void operator*=(const Quaternion &b) {
            w = w * b.w - x * b.x - y * b.y - z * b.z;
            x = w * b.x + x * b.w + y * b.z - z * b.y;
            y = w * b.y + x * b.z + y * b.w + z * b.x;
            z = w * b.z + x * b.y - y * b.x + z * b.w;
        }

        Quaternion operator*(f32 s) const {
            return Quaternion{w * s, x * s, y * s, z * s};
        }

        Quaternion operator/(f32 s) const {
            return Quaternion{w / s, x / s, y / s, z / s};
        }

        /* rotate the quaternion 180 degree */
        Quaternion conjugate() const { return Quaternion{w, -x, -y, -z}; }

        Quaternion inverse() const { return conjugate() / length_square(); }
        Quaternion norm() const {
            f32 len = length();
            return Quaternion{w / len, x / len, y / len, z / len};
        }

        f32 dot(const Quaternion &b) const {
            return w * b.w + x * b.x + y * b.y + z * b.z;
        }

        f32 length_square() const { return w * w + x * x + y * y + z * z; }

        f32 length() const { return sqrtf(length_square()); }

        Vec3 to_euler() {
            return Vec3{
                degree(atanf(2 * (y * z + w * x) / (1 - 2 * (x * x + y * y)))),
                degree(asinf(2 * (x * z - w * y))),
                degree(atanf(2 * (x * y + w * z) / (1 - 2 * (y * y + z * z))))};
        }

        static Quaternion from_euler(f32 x_angle, f32 y_angle, f32 z_angle) {
            f32 x_rad = radians(x_angle / 2);
            f32 y_rad = radians(y_angle / 2);
            f32 z_rad = radians(z_angle / 2);
            f32 sx = sinf(x_rad);
            f32 cx = cosf(x_rad);
            f32 sy = sinf(y_rad);
            f32 cy = cosf(y_rad);
            f32 sz = sinf(z_rad);
            f32 cz = cosf(z_rad);
            return Quaternion{
                cx * cy * cz + sx * sy * sz, sx * cy * cz - cx * sy * sz,
                cx * sy * cz + sx * cy * sz, cx * cy * sz - sx * sy * cz};
        }

        static Quaternion from_euler(Vec3 angles) {
            return from_euler(angles.x, angles.y, angles.z);
        }

        inline static Quaternion identity() { return Quaternion{1, 0, 0, 0}; }

        static Quaternion lerp(const Quaternion &a, const Quaternion &b,
                               f32 t) {
            return a + (b - a) * clampf(t, 0.0, 1.0);
        }

        static Quaternion nlerp(const Quaternion &a, const Quaternion &b,
                                f32 t) {
            f32 dot = a.dot(b);
            if (dot < 0) {
                return lerp(a, b.conjugate(), t).norm();
            } else {
                return lerp(a, b, t).norm();
            }
        }
};

}  // namespace Seed

#endif