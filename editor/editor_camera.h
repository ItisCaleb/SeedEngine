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
            this->cam->set_position(Vec3{0, 1500, 1000});
            this->cam->set_front(0, -50);
            this->cam->set_perspective(45, 1.33, 0.1, 10000.0);
            Input::get_instance()->on_mouse_move(
                [=](f32 last_x, f32 last_y, f32 x, f32 y) {
                    if (!Input::get_instance()->is_mouse_clicked(
                            MouseEvent::RIGHT)) {
                        return;
                    }
                    auto &vp = Seed::RenderEngine::get_instance()->get_layer_viewport(1);
                    auto coord1 = vp.to_viewport_coord(x, y);
                    auto coord2 = vp.to_viewport_coord(last_x, last_y);

                    f32 x_off = coord1.x - coord2.x;
                    f32 y_off = coord2.y - coord1.y;
                    f32 sensitivity = 1000;
                    x_off *= sensitivity;
                    y_off *= sensitivity;
                    Vec3 front = cam->get_front();
                    front.y = 0;
                    front = front.norm();
                    Vec3 pos = cam->get_position();
                    pos -= front * y_off;
                    pos -= front.cross(cam->get_up()) * x_off;
                    cam->set_position(pos);
                });
        }
        ~EditorCamera();
};
#endif