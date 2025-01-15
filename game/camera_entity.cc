#include "camera_entity.h"
#include "core/input.h"
#include "core/render_engine.h"

namespace Seed
{
    void CameraEntity::update(f32 dt){
        Input *input = Input::get_instance();
        Camera *cam = RenderEngine::get_instance()->get_camera();
        Vec3 pos = cam->get_position();
        f32 speed = 5 * dt;
        if(input->is_key_pressed(KeyCode::W)){
            pos += cam->get_front() * speed;
        }

        if(input->is_key_pressed(KeyCode::S)){
            pos -= cam->get_front() * speed;
        }

        if(input->is_key_pressed(KeyCode::D)){
            pos += cam->get_front().cross(cam->get_up()) * speed;
        }

        if(input->is_key_pressed(KeyCode::A)){
            pos -= cam->get_front().cross(cam->get_up()) * speed;
        }
        cam->set_position(pos);
    }
} // namespace Seed
