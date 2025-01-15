#ifndef _SEED_CAMERA_H_
#define _SEED_CAMERA_H_
#include "core/math/vec3.h"
#include "core/math/mat4.h"

namespace Seed {
class Camera {
   private:
    Vec3 position;
    Vec3 up;
    Vec3 front;
    struct {
        bool is_ortho;
        f32 left, right;
        f32 bottom, top;
        f32 near, far;
    } frustum;

   public:
    void set_position(Vec3 pos);
    Vec3 get_position();
    void set_up(Vec3 up);
    Vec3 get_up();
    void set_front(Vec3 front);
    Vec3 get_front();
    void set_frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                     f32 far, bool is_ortho);
    void set_perspective(f64 fovy, f64 aspect, f64 near, f64 far);
    Mat4 look_at();
    Mat4 perspective();
    Camera(Vec3 pos, Vec3 up, Vec3 front);
    ~Camera();
};

}  // namespace Seed

#endif