#ifndef _EDITOR_CAMERA_H_
#define _EDITOR_CAMERA_H_
#include "core/entity.h"
#include "core/rendering/camera.h"
#include "core/input.h"
#include "core/rendering/api/render_engine.h"

using namespace Seed;

class EditorCamera : public Entity {
    private:
        Camera *cam;
        f32 yaw = 0, pitch = 0;

    public:
        void update(f32 dt) {
            Input *input = Input::get_instance();
            Vec3 pos = cam->get_position();
            f32 speed = 1000 * dt;
            Vec3 front = cam->get_front();
            front.y = 0;
            front = front.norm();
            if (input->is_key_pressed(KeyCode::W)) {
                pos += front * speed;
            }

            if (input->is_key_pressed(KeyCode::S)) {
                pos -= front * speed;
            }

            if (input->is_key_pressed(KeyCode::D)) {
                pos += front.cross(cam->get_up()) * speed;
            }

            if (input->is_key_pressed(KeyCode::A)) {
                pos -= front.cross(cam->get_up()) * speed;
            }
            cam->set_position(pos);
        }
        void render() {

        };
        EditorCamera() {
            this->cam = RenderEngine::get_instance()->get_cam();
            this->cam->set_position(Vec3{0, 1000, 1000});
            this->cam->set_front(0, -50);
            this->cam->set_perspective(45, 1.33, 0.1, 10000.0);
        }
        ~EditorCamera();
};
#endif