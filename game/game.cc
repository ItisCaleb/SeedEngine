#include "camera_entity.h"
#include "core/engine.h"
#include "core/math/vec3.h"
#include "core/physic/physic_body.h"
#include "core/physic/physic_shape.h"
#include "core/resource/model.h"
#include "core/resource/resource_loader.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include "core/concurrency/thread_pool.h"
#include "core/os.h"
#include "core/resource/sky.h"
#include "core/gui/gui.h"
#include "core/gui/gui_engine.h"
#include "core/transform.h"
#include "core/world/world.h"
#include "core/world/components.h"
using namespace Seed;

static Vec3 light_dir;

class DebugGUI : public GUI {
    public:
        void update() override {
            auto world = Seed::SeedEngine::get_instance()->get_world();
            ImGui::Begin("Debug");
            light_dir = world->get_direction_light().get_direction();
            static f32 shadow_lambda =
                world->get_direction_light().get_csm_lamda();
            if (ImGui::SliderFloat3("Direction Light",
                                    (float *)(void *)&light_dir, -1.0f, 1.0f)) {
                world->get_direction_light().set_direction(light_dir);
            }
            auto cam = world->get_camera();
            auto cam_pos = cam.get_position();
            ImGui::Text("%.2f %.2f %.2f", cam_pos.x, cam_pos.y, cam_pos.z);
            ImGui::Text("FPS: %.2f", SeedEngine::get_instance()->get_fps());
            ImGui::SliderFloat("Shadow Lambda", &shadow_lambda, 0, 1.0);
            world->get_direction_light().set_csm_lamda(shadow_lambda);
            ImGui::End();
        };
};

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    ResourceLoader *loader = ResourceLoader::get_instance();

    auto terrain =
        loader->load_async<Terrain>("test_project/assets/terrain_01.json");
    auto sky = loader->load_async<Sky>("assets/sky.json");
    auto backpack = loader->load_async<BasicModel>(
        "test_project/assets/backpack.json", [=](Ref<BasicModel> rc) {
            // for (i32 i = 0; i < 10; i++) {
            //     Entity *ent = new Entity(Vec3{(f32)i * 5, 20, (f32)-i});
            //     ent->bind_model(rc);
            //     engine->get_world()->add_entity(ent);
            //     PhysicBoxShape box(Vec3{1, 1, 1});
            //     ent->create_body(box, PhysicBodyType::DYNAMIC);
            // }
        });
    // auto grass = loader->load_async<Billboard>(
    //     "assets/grass.png", [=](Ref<Billboard> rc) {
    //         for (i32 i = 0; i < 10; i++) {
    //             Ref<Transform> tf;
    //             tf.create();
    //             tf->set_position(-i, 20, i);
    //             rc->insert_transform(tf);
    //         }
    //     });
    // auto man = loader->load_async<SkeletonModel>(
    //     "test_project/assets/man.json", [=](Ref<SkeletonModel> rc) {
    //         Entity *ent = new Entity(Vec3{0, 0, 0});
    //         ent->get_transform()->set_scale(Vec3{0.1, 0.1, 0.1});
    //         PhysicBoxShape box(Vec3{1, 1, 1});
    //         ent->create_body(box, PhysicBodyType::DYNAMIC);
    //         ent->bind_skeleton_model(rc);
    //         ent->play_animation("Take 001");
    //         engine->get_world()->add_entity(ent);
    //     });

    GuiEngine::get_instance()->add_gui(new DebugGUI);
    World *world = engine->get_world();
    world->get_point_lights().push_back(
        PointLight{Vec3{2, 10, 2}, Vec3{0.8, 0.5, 0.5}, Vec3{}});
    // world->add_entity<CameraEntity>();
    world->set_sky(sky->wait());
    // world->add_billboard(grass->wait());
    Ref<WorldChunk> chunk;
    chunk.create(terrain->wait());
    Transform t;
    t.set_position(10, 10, 10);
    auto model = backpack->wait();
    chunk->add_object<BasicModel>(t, PhysicShape{}, model, t);
    world->add_chunk(chunk);
    auto &ecs = world->ecs();
    Entity a = ecs.create_entity();
    PhysicBoxShape box(Vec3{1, 1, 1});
    t.set_position(5, 10, 5);
    ecs.add_component<Transform>(a, t);
    ecs.add_component<PhysicBody>(a, box, PhysicBodyType::DYNAMIC);
    ecs.add_component<MeshInstance>(a, MeshInstance{.model = model});
    CameraEntity::create_entity(ecs);
    engine->start();

    return 0;
}