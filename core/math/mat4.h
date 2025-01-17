#ifndef _SEED_Mat4_H_
#define _SEED_Mat4_H_
#include "vec4.h"
#include <fmt/format.h>

namespace Seed {
struct Mat4 {
    Vec4 data[4] = {Vec4{1, 0, 0, 0}, Vec4{0, 1, 0, 0}, Vec4{0, 0, 1, 0},
                    Vec4{0, 0, 0, 1}};

    Vec4 &operator[](i32 row) { return data[row]; }
    const Vec4 &operator[](i32 row) const { return data[row]; }

    Mat4 operator+(const Mat4 &b) {
        return Mat4{data[0] + b[0], data[1] + b[1], data[2] + b[2],
                    data[3] + b[3]};
    }

    void operator+=(const Mat4 &b) {
        data[0] += b[0];
        data[1] += b[1];
        data[2] += b[2];
        data[3] += b[3];
    }

    Mat4 operator-(const Mat4 &b) {
        return Mat4{data[0] - b[0], data[1] - b[1], data[2] - b[2],
                    data[3] - b[3]};
    }

    Mat4 operator-() { return Mat4{-data[0], -data[1], -data[2], -data[3]}; }

    void operator-=(const Mat4 &b) {
        data[0] -= b[0];
        data[1] -= b[1];
        data[2] -= b[2];
        data[3] -= b[3];
    }

    Mat4 operator*(const f32 s) {
        return Mat4{data[0] * s, data[1] * s, data[2] * s, data[3] * s};
    }

    void operator*=(const f32 s) {
        data[0] *= s;
        data[1] *= s;
        data[2] *= s;
    }

    Mat4 operator*(const Mat4 &b) {
        Mat4 tb = b.transpose();
        return Mat4{data[0].dot(tb[0]), data[0].dot(tb[1]), data[0].dot(tb[2]),
                    data[0].dot(tb[3]), data[1].dot(tb[0]), data[1].dot(tb[1]),
                    data[1].dot(tb[2]), data[1].dot(tb[3]), data[2].dot(tb[0]),
                    data[2].dot(tb[1]), data[2].dot(tb[2]), data[2].dot(tb[3]),
                    data[3].dot(tb[0]), data[3].dot(tb[1]), data[3].dot(tb[2]),
                    data[3].dot(tb[3])};
    }

    Vec4 operator*(const Vec4 &b) {
        return Vec4{data[0].dot(b), data[1].dot(b), data[2].dot(b),
                    data[3].dot(b)};
    }

    void operator*=(const Mat4 &b) {
        Mat4 tb = b.transpose();
        data[0][0] = data[0].dot(tb[0]);
        data[0][1] = data[0].dot(tb[1]);
        data[0][2] = data[0].dot(tb[2]);
        data[0][3] = data[0].dot(tb[3]);
        data[1][0] = data[1].dot(tb[0]);
        data[1][1] = data[1].dot(tb[1]);
        data[1][2] = data[1].dot(tb[2]);
        data[1][3] = data[1].dot(tb[3]);
        data[2][0] = data[2].dot(tb[0]);
        data[2][1] = data[2].dot(tb[1]);
        data[2][2] = data[2].dot(tb[2]);
        data[2][3] = data[2].dot(tb[3]);
        data[3][0] = data[3].dot(tb[0]);
        data[3][1] = data[3].dot(tb[1]);
        data[3][2] = data[3].dot(tb[2]);
        data[3][3] = data[3].dot(tb[3]);
    }

    Mat4 transpose() const {
        return Mat4{this->data[0][0], this->data[1][0], this->data[2][0],
                    this->data[3][0], this->data[0][1], this->data[1][1],
                    this->data[2][1], this->data[3][1], this->data[0][2],
                    this->data[1][2], this->data[2][2], this->data[3][2],
                    this->data[0][3], this->data[1][3], this->data[2][3],
                    this->data[3][3]};
    }

    inline static Mat4 translate_mat(Vec3 t) {
        return Mat4{Vec4{1, 0, 0, t.x}, Vec4{0, 1, 0, t.y}, Vec4{0, 0, 1, t.z},
                    Vec4{0, 0, 0, 1}};
    }

    Mat4 translate(Vec3 t) { return Mat4::translate_mat(t) * (*this); }

    inline static Mat4 scale_mat(Vec3 s) {
        return Mat4{Vec4{s.x, 0, 0, 0}, Vec4{0, s.y, 0, 0}, Vec4{0, 0, s.z, 0},
                    Vec4{0, 0, 0, 1}};
    }

    Mat4 scale(Vec3 s) { return Mat4::scale_mat(s) * (*this); }

    inline static Mat4 coord_transform_mat(Vec3 u, Vec3 v, Vec3 w) {
        return Mat4{Vec4{u.x, u.y, u.z, 0}, Vec4{v.x, v.y, v.z, 0},
                    Vec4{w.x, w.y, w.z, 0}, Vec4{0, 0, 0, 1}};
    }

    inline static Mat4 rotate_mat(f32 rad, Vec3 axis) {
        f32 s = sinf(rad);
        f32 c = cosf(rad);
        f32 comp_c = 1 - c;
        f32 x = axis.x;
        f32 y = axis.y;
        f32 z = axis.z;
        return Mat4{Vec4{c + x * x * comp_c, x * y * comp_c - z * s,
                         x * z * comp_c + y * s, 0},
                    Vec4{y * x * comp_c + z * s, c + y * y * comp_c,
                         y * z * comp_c - x * s, 0},
                    Vec4{z * x * comp_c - y * s, z * y * comp_c + x * s,
                         c + z * z * comp_c, 0},
                    Vec4{0, 0, 0, 1}};
    }

    Mat4 rotate(f32 rad, Vec3 axis) {
        return Mat4::rotate_mat(rad, axis) * (*this);
    }
};

}  // namespace Seed

template <>
class fmt::formatter<Seed::Mat4> {
   public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(Seed::Mat4 const &mat, Context &ctx) const {
        return format_to(
            ctx.out(),
            "{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n-----",
            mat[0].x, mat[0].y, mat[0].z, mat[0].w, mat[1].x, mat[1].y,
            mat[1].z, mat[1].w, mat[2].x, mat[2].y, mat[2].z, mat[2].w,
            mat[3].x, mat[3].y, mat[3].z, mat[3].w);
    }
};

#endif