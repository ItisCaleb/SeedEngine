#ifndef _SEED_TERRAIN_GUI_H_
#define _SEED_TERRAIN_GUI_H_
#include "core/gui/gui.h"
#include "editor_terrain.h"

using namespace Seed;
class TerrainGUI : public Seed::GUI {
    private:
        Ref<EditorTerrain> terrain;
        Ref<MappableTexture> terrain_pos_tex;

    public:
        void update() override;
        TerrainGUI();
};

#endif