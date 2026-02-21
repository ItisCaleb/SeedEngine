#include "core/engine.h"
#include "core/resource/resource_loader.h"
#include "core/rendering/api/render_engine.h"
#include "camera_entity.h"
#include <spdlog/spdlog.h>
#include "core/concurrency/thread_pool.h"
#include "core/os.h"
#include "core/resource/sky.h"
#include "core/debug/debug_drawer.h"
#include "core/gui/gui.h"
#include "core/gui/gui_engine.h"
#include "core/rendering/shadow_map.h"
#include "core/input.h"

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
static Vec3 light_dir;

class DebugGUI : public GUI {
    public:
        void update() override {
            auto world = Seed::SeedEngine::get_instance()->get_world();
            ImGui::Begin("Debug");
            light_dir = world->get_direction_light().get_direction();
            if (ImGui::SliderFloat3("Direction Light",
                                    (float *)(void *)&light_dir, -1.0f, 1.0f)) {
                world->get_direction_light().set_direction(light_dir);
            }
            auto cam = RenderEngine::get_instance()->get_cam();
            auto cam_pos = cam->get_position();
            ImGui::Text("%.2f %.2f %.2f", cam_pos.x, cam_pos.y, cam_pos.z);
            ImGui::Text("FPS: %.2f", SeedEngine::get_instance()->get_fps());
            // ImGui::SliderFloat("CSM Lambda", &Camera::shadow_lamdba, 0, 1.0);
            if (ImGui::Button("ortho")) {
                cam->set_ortho(-10, 10, -10, 10, -100, 100);
                // set position from origin
                Vec3 pos_dir = Vec3{-0.5, -0.5, 0};
                cam->set_position(-pos_dir);
                cam->set_front(pos_dir);
            }
            if (ImGui::Button("Terrain vertex")) {
                auto mat = world->get_terrain()->get_material();
                auto state = mat->get_rasterizer_state();
                if (state.poly_mode == PolygonMode::FILL) {
                    state.poly_mode = PolygonMode::LINE;
                } else {
                    state.poly_mode = PolygonMode::FILL;
                }

                //mat->set_rasterizer_state(state);
            }
            ImGui::End();
        };
};

class InputGUI : public GUI {
public:
    // 必須確保與基底類別簽署一致
    void update() override {
        ImGui::Begin("Input Settings");

        auto input = Seed::Input::get_instance();
        if (!input) {
            ImGui::Text("No Input Instance");
            ImGui::End();
            return;
        }

        static std::string waiting_for_action = "";

        // 這裡列出你想要自定義的按鍵
        // 注意：這裡假設你的 Input 類別已經有了 bindings 映射表
        // 如果沒有，請參考下方的「應急方案」
        for (auto& [action_name, bound_code] : input->bindings) {
            ImGui::Text("%-15s", action_name.c_str());
            ImGui::SameLine();

            std::string label = (waiting_for_action == action_name) 
                                ? "Press any key..." 
                                : fmt::format("Key: {}", (char)bound_code);

            if (ImGui::Button(label.c_str(), ImVec2(150, 0))) {
                waiting_for_action = action_name;
            }

            if (waiting_for_action == action_name) {
                // 遍歷常用 KeyCode
                for (int i = (int)KeyCode::SPACE; i <= (int)KeyCode::QUOTELEFT; i++) {
                    if (ImGui::IsKeyPressed((ImGuiKey)i)) {
                        bound_code = static_cast<KeyCode>(i);
                        waiting_for_action = "";
                        break;
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) waiting_for_action = "";
            }
        }
        ImGui::End();
    }
};

int main(void) {
    SeedEngine *engine = new SeedEngine(60.0f);
    ResourceLoader *loader = ResourceLoader::get_instance();

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    // for (int i = 0; i < 6; i++) {
    //     for (int j = 0; j < 6; j++) {
    //         vertices.push_back(
    //             ModelVertex{CUBE[CUBE_INDICE[i][j]], CUBE_NORMAL[i],
    //             CUBE_TEX[j]});
    //     }
    // }

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
            for (i32 i = 0; i < 10; i++) {
                Entity *ent = new Entity(Vec3{(f32)i * 5, 20, (f32)-i});
                ent->bind_model(rc);
                engine->get_world()->add_entity(ent);
                PhysicBoxShape box(Vec3{1, 1, 1});
                ent->create_body(box, PhysicBodyType::DYNAMIC);
            }
        });
    auto grass = loader->load_async<Billboard>(
        "assets/grass.png", [=](Ref<Billboard> rc) {
            for (i32 i = 0; i < 10; i++) {
                Ref<Transform> tf;
                tf.create();
                tf->set_position(-i, 20, i);
                rc->insert_transform(tf);
            }
        });

    // load gui
    GuiEngine::get_instance()->add_gui(new DebugGUI);
    GuiEngine::get_instance()->add_gui(new InputGUI);
    World *world = engine->get_world();
    world->get_point_lights().push_back(
        PointLight{Vec3{2, 10, 2}, Vec3{0.8, 0.5, 0.5}, Vec3{}});
    world->add_entity<CameraEntity>();
    world->set_sky(sky->wait());
    world->add_billboard(grass->wait());
    engine->start();

    return 0;
}
