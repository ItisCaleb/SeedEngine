#ifndef _SEED_TERRAIN_EDITOR_H_
#define _SEED_TERRAIN_EDITOR_H_
#include "editor/terrain/editor_terrain.h"
#include "editor/terrain/terrain_editor_renderer.h"

namespace Seed {
class TerrainEditor {
    friend TerrainEditorRenderer;
    private:
        Ref<Texture> screen_texture;
        Ref<Texture> screen_depth;
        Ref<MappableTexture> picking_texture;
        Camera camera;
        Ref<EditorTerrain> current_terrain;
        TerrainEditorRenderer *renderer;
        u32 screen_width = 1024;
        u32 screen_height = 768;
        void edit_terrain();
    public:
        void init();
        void update();
};
}  // namespace Seed

#endif