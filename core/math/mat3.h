#ifndef _SEED_MAT3_H_
#define _SEED_MAT3_H_
#include "vec3.h"

namespace Seed {
struct Mat3 {
    Vec3 data[3] = {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}};

    Vec3 &operator[](i32 row) { return data[row]; }
    const Vec3 &operator[](i32 row) const { return data[row]; }

    Mat3 operator+(const Mat3 &b) {
        return Mat3(data[0] + b[0], data[1] + b[1], data[2] + b[2]);
    }

    void operator+=(const Mat3 &b) {
        data[0] += b[0];
        data[1] += b[1];
        data[2] += b[2];
    }

    Mat3 operator-(const Mat3 &b) {
        return Mat3(data[0] - b[0], data[1] - b[1], data[2] - b[2]);
    }

    Mat3 operator-() { return Mat3{-data[0], -data[1], -data[2]}; }

    void operator-=(const Mat3 &b) {
        data[0] -= b[0];
        data[1] -= b[1];
        data[2] -= b[2];
    }

    Mat3 operator*(const f32 s) {
        return Mat3{data[0] * s, data[1] * s, data[2] * s};
    }

    void operator*=(const f32 s) {
        data[0] *= s;
        data[1] *= s;
        data[2] *= s;
    }

    Mat3 operator*(const Mat3 &b) {
        Mat3 tb = b.transpose();
        return Mat3(data[0].dot(tb[0]), data[0].dot(tb[1]), data[0].dot(tb[2]),
                    data[1].dot(tb[0]), data[1].dot(tb[1]), data[1].dot(tb[2]),
                    data[2].dot(tb[0]), data[2].dot(tb[1]), data[2].dot(tb[2]));
    }

    Vec3 operator*(const Vec3 &b) {
        return Vec3{data[0].dot(b), data[1].dot(b), data[2].dot(b)};
    }

    void operator*=(const Mat3 &b) {
        Mat3 tb = b.transpose();
        data[0][0] = data[0].dot(tb[0]);
        data[0][1] = data[0].dot(tb[1]);
        data[0][2] = data[0].dot(tb[2]);
        data[1][0] = data[1].dot(tb[0]);
        data[1][1] = data[1].dot(tb[1]);
        data[1][2] = data[1].dot(tb[2]);
        data[2][0] = data[2].dot(tb[0]);
        data[2][1] = data[2].dot(tb[1]);
        data[2][2] = data[2].dot(tb[2]);
    }

    Mat3 transpose() const {
        return Mat3(this->data[0][0], this->data[1][0], this->data[2][0],
                    this->data[0][1], this->data[1][1], this->data[2][1],
                    this->data[0][2], this->data[1][2], this->data[2][2]);
    }

    Mat3(Vec3 row1, Vec3 row2, Vec3 row3) {
        this->data[0] = row1;
        this->data[1] = row2;
        this->data[2] = row3;
    }

    Mat3(f32 d00, f32 d01, f32 d02, f32 d10, f32 d11, f32 d12, f32 d20, f32 d21,
         f32 d22)
        : Mat3(Vec3{d00, d01, d02}, Vec3{d10, d11, d12}, Vec3{d20, d21, d22}) {}
};

}  // namespace Seed

#endif