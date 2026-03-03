#include "asset_viewer.h"
#include <imgui.h>
#include <nfd.h>
#include "editor/editor.h"

namespace Seed {
void AssetViewer::init() {}
void AssetViewer::update() {
    ImGui::BeginGroup();
    if (ImGui::Button("Open model file")) {
        if (current_model) {
            delete current_model;
        }
        nfdu8char_t *path;
        nfdopendialogu8args_t args = {0};
        nfdresult_t r = NFD_OpenDialogU8_With(&path, &args);
        if (r == NFD_OKAY) {
            current_model = new EditorModel(path);
        }
    }

    if (ImGui::Button("Dump model")) {
        if (current_model != nullptr) {
            current_model->dump(gEditor->project()->get_asset_dir());
            std::string path = 
                fmt::format("{}/test.mdl", current_model->directory);

            // ResourceLoader *loader = ResourceLoader::get_instance();
            // loader->load_async<Seed::Model>(path, [=](Ref<Seed::Model> rc) {
            //     // ModelEntity *ent = new ModelEntity(Vec3{0, 0, -5}, rc);
            //     // auto engine = SeedEngine::get_instance();
            //     // engine->get_world()->add_entity(ent);
            //     // engine->get_world()->add_model_entity(ent);
            // });
        }
    }
    if (current_model != nullptr) {
        ImGui::Text("mesh count: %zu", current_model->meshes.size());
        ImGui::Text("bone mesh count: %zu", current_model->bone_meshes.size());
        ImGui::Text("texture count: %zu", current_model->textures.size());
    }
    ImGui::EndGroup();
}
}  // namespace Seed