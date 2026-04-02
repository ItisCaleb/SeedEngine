#ifndef _SEED_TERRAIN_EDITOR_H_
#define _SEED_TERRAIN_EDITOR_H_
#include "editor/world/editor_terrain.h"
#include "editor/world/terrain_editor_renderer.h"
#include <imgui.h>
#include <string>

namespace Seed {

enum class TerrainTool {
    Raise,
    Lower,
    Smooth,
    Flatten,
    Noise,
    Erode,
    Paint,
    Pick
};

struct SplatLayer {
        std::string name;
        Ref<Texture> albedo;
        int channel;  // 0=R 1=G 2=B 3=A
        float weight;
        ImVec4 preview_color;
};

class TerrainEditor {
        friend TerrainEditorRenderer;

    private:
        Ref<Texture> screen_texture;
        Ref<Texture> screen_depth;
        Ref<MappableTexture> picking_texture;
        Ref<EditorTerrain> current_terrain;
        TerrainEditorRenderer *renderer;
        u32 screen_width = 1024;
        u32 screen_height = 768;
        i16 last_click_x, last_click_y;

        TerrainTool active_tool = TerrainTool::Raise;
        int active_layer = 0;
        float brush_radius = 10.f;
        float brush_strength = 0.3f;
        float brush_falloff = 0.6f;
        float brush_opacity = 0.8f;
        float paint_flow = 0.5f;
        float paint_hardness = 0.7f;
        bool slope_blend = true;
        bool height_blend = false;
        float blend_threshold = 0.45f;
        float tile_u = 4.f;
        float tile_v = 4.f;
        bool normal_map = true;
        bool lightmap_baking = false;
        /* terrain creation */
        bool show_new_terrain_modal = false;
        char new_terrain_name[64] = "terrain_01";
        int new_terrain_w = 512;
        int new_terrain_h = 512;
        Ref<MappableTexture> new_terrain_heightmap;
        bool init_flat = true;
        bool init_noise = false;
        std::vector<SplatLayer> splat_layers;
        void init_default_splat_layers();
        void draw_right_panel();
        void apply_brush(i16 cx, i16 cy);
        void edit_terrain_imgui(ImVec2 origin, float w, float h);
        void draw_viewport(float vp_w, float vp_h);
        void draw_left_panel();
        void draw_new_terrain_modal();
        const char *tool_name(TerrainTool t);
        void load_terrain(const std::string& path);
    public:
        void init();
        void update();
};
}  // namespace Seed

#endif