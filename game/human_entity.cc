#include "human_entity.h"
#include "core/entity.h"
#include "core/math/vec3.h"
#include "core/input.h"
#include "core/engine.h"


namespace Seed {
HumanEntity::HumanEntity(Vec3 position) : Entity(position) {
    // transform->set_position()
    // transform->set_rotation()
}

void HumanEntity::update(f32 dt) {
    // Input *input = Input::get_instance();

    // Camera *cam = &SeedEngine::get_instance()->get_world()->get_camera();

    // Vec3 pos = transform->get_position();
    // f32 speed = 50 * dt;

    // // ===== 方向（依 camera yaw）=====
    // Vec3 forward = cam->get_front();
    // forward.y = 0;
    // forward = forward.norm();

    // Vec3 right = forward.cross(cam->get_up()).norm();

    // // ===== 移動 =====
    // if (input->is_key_pressed(KeyCode::W)) {
    //     pos += forward * speed;
    // }
    // if (input->is_key_pressed(KeyCode::S)) {
    //     pos -= forward * speed;
    // }
    // if (input->is_key_pressed(KeyCode::D)) {
    //     pos += right * speed;
    // }
    // if (input->is_key_pressed(KeyCode::A)) {
    //     pos -= right * speed;
    // }

    // transform->set_position(pos);


    // if (move_dir.length() > 0.01f) {
    //     move_dir = move_dir.norm();
    //     pos += move_dir * speed;

    //     // 記錄角色面向
    //     forward = move_dir;
    // }

    // transform->set_position(pos);

    // // ===== Camera 跟隨 =====
    // Vec3 cam_pos = pos - forward * 20.0f + Vec3{0, 10, 0};
    // cam->set_position(cam_pos);

    // // 看向角色（自己算 front）
    // Vec3 cam_front = (pos + Vec3{0, 5, 0}) - cam_pos;
    // cam->set_front(cam_front.norm());
}

}  // namespace Seed