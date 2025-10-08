#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include "core/rendering/api/render_engine.h"
#include "camera_entity.h"
#include "core/model_entity.h"
#include <spdlog/spdlog.h>
#include "core/concurrency/thread_pool.h"
#include "core/os.h"
#include "core/resource/sky.h"
#include "core/debug/debug_drawer.h"
#include "core/gui/gui.h"
#include "core/gui/gui_engine.h"
#include "core/rendering/shadow_map.h"

using namespace Seed;

static const Vec3 CUBE[] = {
    /* down-left */
    {-0.5f, -0.5f, -0.5f},
    /* down-right */
    {0.5f, -0.5f, -0.5f},
    /* top-right */
    {0.5f, 0.5f, -0.5f},
    /* top-left */
    {-0.5f, 0.5f, -0.5f},
    /* down-left */
    {-0.5f, -0.5f, 0.5f},
    /* down-right */
    {0.5f, -0.5f, 0.5f},
    /* top-right */
    {0.5f, 0.5f, 0.5f},
    /* top-left */
    {-0.5f, 0.5f, 0.5f},
};

static const u32 CUBE_INDICE[6][6] = {
    // Vertices according to faces
    /* front */
    {0, 1, 2, 2, 3, 0},
    /* back */
    {5, 4, 7, 7, 6, 5},
    /* left */
    {4, 0, 3, 3, 7, 4},
    /* right */
    {1, 5, 6, 6, 2, 1},
    /* down */
    {4, 0, 1, 1, 5, 4},
    /* top */
    {3, 2, 6, 6, 7, 3}};

static const Vec3 CUBE_NORMAL[] = {0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, 1.0f,
                                   -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
                                   0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, 0.0f};

static const Vec2 CUBE_TEX[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                                1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};

class DebugGUI : public GUI {
    public:
        void update() override {
            auto world = Seed::SeedEngine::get_instance()->get_world();
            ImGui::Begin("Debug");
            if(ImGui::SliderFloat3(
                "Direction Light",
                (float *)(void *)&world->get_direction_light().get_position(),
                -1.0f, 1.0f)){
                    world->get_direction_light().set_dirty();
                }
            auto cam = RenderEngine::get_instance()->get_cam();
            auto cam_pos = cam->get_position();
            ImGui::Text("%.2f %.2f %.2f",  cam_pos.x, cam_pos.y, cam_pos.z);
            if(ImGui::Button("ortho")){
                cam->set_ortho(-10, 10, -10, 10, -100, 100);
                // set position from origin
                Vec3 pos_dir =  Vec3{-0.5, -0.5, 0};
                cam->set_position(-pos_dir);
                cam->set_front(pos_dir);
            }
            ImGui::End();
        };
};

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    ResourceLoader *loader = ResourceLoader::get_instance();

    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            vertices.push_back(
                Vertex{CUBE[CUBE_INDICE[i][j]], CUBE_NORMAL[i], CUBE_TEX[j]});
        }
    }

    for (int i = 0; i < 36; i++) {
        indices.push_back(i);
    }

    // Mesh mesh(vertices, indices);
    // std::vector<Mesh> meshs = {mesh};

    // Ref<Model> model = Model::create(meshs, mats, {});
    //  for (int i = -100; i < 100; i++) {
    //      for (int j = -100; j < 100; j++) {
    //          ModelEntity *ent = new ModelEntity(Vec3{(f32)i, (f32)j, 0},
    //          mesh); ent->set_material(mat); ent->set_scale({0.1, 0.1, 0.1});
    //          engine->get_world()->add_entity(ent);
    //          engine->get_world()->add_model_entity(ent);
    //      }
    //  }

    auto terrain = loader->load_async<Terrain>("assets/iceland_heightmap.png");
    auto sky = loader->load_async<Sky>("assets/sky.json");
    auto backpack = loader->load_async<Model>(
        "assets/backpack/test.mdl", [=](Ref<Model> rc) {
            ModelEntity *ent = new ModelEntity(Vec3{0, 20, -5}, rc);
            engine->get_world()->add_entity(ent);
            engine->get_world()->add_model_entity(ent);
            PhysicBoxShape box(ent->get_model_aabb().ext);
            ent->create_body(box, PhysicBodyType::DYNAMIC);
        });
    GuiEngine::get_instance()->add_gui(new DebugGUI);
    engine->get_world()->get_point_lights().push_back(
        Light{LightType::POINT, Vec3{2, 10, 2}, Vec3{0.8, 0.5, 0.5}, Vec3{}});
    engine->get_world()->add_entity<CameraEntity>();
    engine->get_world()->set_terrain(terrain->wait());
    engine->get_world()->set_sky(sky->wait());
    engine->start();

    return 0;
}