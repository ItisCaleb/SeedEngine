#ifndef _SEED_GUI_ENGINE_H_
#define _SEED_GUI_ENGINE_H_
#include <RmlUi/Core/Context.h>
#include "core/gui/rml_interface.h"
#include <unordered_map>
#include <vector>

namespace Seed {

class ImGUI;
class RmlGUI;
class RmlRenderer;
class Window;
class GuiEngine {
        friend RmlRenderer;

    private:
        std::vector<ImGUI *> guis;
        std::unordered_map<KString, Ref<Texture>> stored_texture;
        RmlGUI *rml_root = nullptr;
        Window *window = nullptr;
        SeedRmlElementInstancer rml_element_instancer;
        SeedRmlSystemInterface rml_system_interface;
        SeedRmlRenderInterface rml_interface;
        Rml::Context *rml_context = nullptr;

        void update_rml_context();
        void register_seed_rml(Rml::Context *context);
        SeedRmlRenderInterface *get_rml_render_interface() {
            return &this->rml_interface;
        }

    public:
        void update();
        Rml::Context *get_rml_context() { return rml_context; }
        void add_imgui(ImGUI *gui);
        void remove_imgui(ImGUI *gui);
        void load_rmlui(RmlGUI *gui);

        /* add a texture to allow gui to use */
        /* e.g. internal://name */
        void add_texture(KStr name, Ref<Texture> texture);
        Ref<Texture> get_texture(KStr name);

        GuiEngine(Window *window);
        ~GuiEngine();
};
}  // namespace Seed

#endif
