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
        Camera camera;
        Ref<EditorTerrain> current_terrain;
        TerrainEditorRenderer *renderer;

    public:
        void init();
        void update();
};
}  // namespace Seed

#endif