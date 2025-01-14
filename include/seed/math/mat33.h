#ifndef _SEED_MAT33_H_
#define _SEED_MAT33_H_
#include <seed/math/vec3.h>

namespace Seed {
struct Mat33 {
    Vec3 data[3] = {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}};

    Vec3 &operator[](i32 row) { return data[row]; }
    const Vec3 &operator[](i32 row) const { return data[row]; }

    Mat33 operator+(const Mat33 &b) {
        return Mat33(data[0] + b[0], data[1] + b[1], data[2] + b[2]);
    }

    void operator+=(const Mat33 &b) {
        data[0] += b[0];
        data[1] += b[1];
        data[2] += b[2];
    }

    Mat33 operator-(const Mat33 &b) {
        return Mat33(data[0] - b[0], data[1] - b[1], data[2] - b[2]);
    }

    Mat33 operator-() { return Mat33{-data[0], -data[1], -data[2]}; }

    void operator-=(const Mat33 &b) {
        data[0] -= b[0];
        data[1] -= b[1];
        data[2] -= b[2];
    }

    Mat33 operator*(const f32 s) {
        return Mat33{data[0] * s, data[1] * s, data[2] * s};
    }

    void operator*=(const f32 s) {
        data[0] *= s;
        data[1] *= s;
        data[2] *= s;
    }

    Mat33 operator*(const Mat33 &b) {
        Mat33 tb = b.transpose();
        return Mat33(data[0] * tb[0], data[0] * tb[1], data[0] * tb[2],
                     data[1] * tb[0], data[1] * tb[1], data[1] * tb[2],
                     data[2] * tb[0], data[2] * tb[1], data[2] * tb[2]);
    }

    void operator*=(const Mat33 &b) {
        Mat33 tb = b.transpose();
        data[0][0] = data[0] * tb[0];
        data[0][1] = data[0] * tb[1];
        data[0][2] = data[0] * tb[2];
        data[1][0] = data[1] * tb[0];
        data[1][1] = data[1] * tb[1];
        data[1][2] = data[1] * tb[2];
        data[2][0] = data[2] * tb[0];
        data[2][1] = data[2] * tb[1];
        data[2][2] = data[2] * tb[2];
    }

    Mat33 transpose() const {
        return Mat33(this->data[0][0], this->data[1][0], this->data[2][0],
                     this->data[0][1], this->data[1][1], this->data[2][1],
                     this->data[0][2], this->data[1][2], this->data[2][2]);
    }

    Mat33(Vec3 row1, Vec3 row2, Vec3 row3) {
        this->data[0] = row1;
        this->data[1] = row2;
        this->data[2] = row3;
    }

    Mat33(f32 d00, f32 d01, f32 d02, f32 d10, f32 d11, f32 d12, f32 d20,
          f32 d21, f32 d22)
        : Mat33(Vec3{d00, d01, d02}, Vec3{d10, d11, d12}, Vec3{d20, d21, d22}) {

    }
};

}  // namespace Seed

#endif