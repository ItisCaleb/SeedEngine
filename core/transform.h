#ifndef _SEED_TRANSFORM_H_
#define _SEED_TRANSFORM_H_
#include "core/math/vec3.h"
#include "core/math/mat4.h"
#include "core/collision/shape.h"

namespace Seed {
class World;
class Transform {
        friend World;

    private:
        Vec3 position = Vec3{0, 0, 0};
        Quaternion rotation = Quaternion::identity();
        Vec3 scale = Vec3{1, 1, 1};
        Mat4 model_matrix;
        bool dirty = true;
        void update() {
            this->model_matrix = Mat4::translate_mat(position) *
                                 Mat4::rotate_mat(rotation) *
                                 Mat4::scale_mat(scale);
        }

    public:
        Vec3 get_position() const { return position; }
        void set_position(const Vec3 &position) {
            this->position = position;
            this->dirty = true;
        }
        void set_position(f32 x, f32 y, f32 z) { set_position(Vec3{x, y, z}); }

        Quaternion get_rotation() const { return rotation; }
        void set_rotation(const Quaternion &rotation) {
            this->rotation = rotation;
            this->dirty = true;
        }
        Vec3 get_scale() const { return scale; }
        void set_scale(const Vec3 &scale) {
            this->scale = scale;
            this->dirty = true;
        }
        void rotate(f32 x_angle, f32 y_angle, f32 z_angle) {
            this->rotation *= Quaternion::from_euler(x_angle, y_angle, z_angle);
            this->dirty = true;
        }

        void rotate(const Quaternion &rotation) {
            this->rotation *= rotation;
            this->dirty = true;
        }

        Mat4 get_model_matrix() {
            if (dirty) {
                update();
                this->dirty = false;
            }
            return this->model_matrix;
        }

        AABB translate_AABB(const AABB &aabb) const {
            AABB result;
            Mat4 rot_mat = Mat4::rotate_mat(rotation);
            result.center = position;
            result.ext = {0, 0, 0};
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    result.center[i] +=
                        rot_mat[i][j] * aabb.center[j] * scale[j];
                    result.ext[i] +=
                        abs(rot_mat[i][j]) * aabb.ext[j] * scale[j];
                }
            }
            return result;
        }
};

}  // namespace Seed

#endif