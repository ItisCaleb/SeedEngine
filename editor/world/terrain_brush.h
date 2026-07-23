#ifndef _SEED_TERRAIN_BRUSH_H_
#define _SEED_TERRAIN_BRUSH_H_

#include "terrain_tile_map.h"

namespace Seed {

enum class TerrainBrush { Raise, Lower, Smooth, Flatten, Pick, Splat };

struct TerrainBrushSetting {
        u32 radius = 12;
        u32 strength = 25;
        u32 flatten_height = 128;
        i32 terrain_palette_slot = -1;
};

TerrainEdit apply_terrain_brush(TerrainTileMap &tiles, i32 x, i32 y,
                                TerrainBrush brush,
                                const TerrainBrushSetting &setting);

}  // namespace Seed

#endif
