#ifndef _SEED_WORLD_EDITOR_H_
#define _SEED_WORLD_EDITOR_H_

#include <memory>
#include <string>
#include "core/io/path.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/texture.h"
#include "editor/world/editor_world.h"

namespace Seed {
class WorldRenderer;

class WorldEditor {
    friend WorldRenderer;
    private:
        std::unique_ptr<EditorWorld> current_world;
        Ref<Texture> screen_texture;
        Ref<Texture> screen_depth;
        Ref<Texture> picking_texture;
        ResourceConfiguration standalone_config;
        WorldRenderer *renderer = nullptr;
        ResourceEntry *current_entry = nullptr;
        Path current_world_path;
        bool current_world_from_entry = false;
        u32 screen_width = 1024;
        u32 screen_height = 768;
        i32 selected_chunk = -1;
        bool preview_terrain_dirty = false;
        std::string status_text;

        ResourceEntry *find_entry_for_path(const Path &path);
        void set_current_world_inspector();
        void validate_selected_chunk();
        void mark_preview_terrain_dirty();
        void draw_left_panel();
        void draw_center_panel();
        void draw_viewport(float viewport_w, float viewport_h);
        void draw_right_panel();
        void draw_uuid_field(const char *label, UUID &uuid);
        void draw_vec3_field(const char *label, Vec3 &value);
        void draw_static_objects(EditorChunk &chunk);
        void draw_point_lights(EditorChunk &chunk);
        void add_chunk();
        void remove_selected_chunk();
        void save_current_world();

    public:
        void init();
        void update();
        bool load_world(const Path &path);
        EditorWorld *get_current_world() { return current_world.get(); }
};

}  // namespace Seed

#endif
