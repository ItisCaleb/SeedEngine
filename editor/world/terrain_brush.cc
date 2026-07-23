#include "terrain_brush.h"

#include <algorithm>
#include <cmath>

namespace Seed {

namespace {

bool apply_brush_sample(TerrainTileMap &tiles, i32 x, i32 y, TerrainBrush brush,
                        f32 amount, const TerrainBrushSetting &setting,
                        TerrainEdit &edit) {
    if (amount <= 0.0f) return false;

    u8 current = 0;
    switch (brush) {
        case TerrainBrush::Raise:
            return tiles.read_height(x, y, current) &&
                   tiles.write_height(
                       x, y, current + (i32)std::round(255.0f * amount), edit);
        case TerrainBrush::Lower:
            return tiles.read_height(x, y, current) &&
                   tiles.write_height(
                       x, y, current - (i32)std::round(255.0f * amount), edit);
        case TerrainBrush::Flatten:
            return tiles.read_height(x, y, current) &&
                   tiles.write_height(
                       x, y,
                       current + (i32)std::round(
                                     ((i32)setting.flatten_height - current) *
                                     amount),
                       edit);
        case TerrainBrush::Smooth: {
            if (!tiles.read_height(x, y, current)) return false;

            i32 sum = 0;
            i32 count = 0;
            for (i32 offset_y = -1; offset_y <= 1; offset_y++) {
                for (i32 offset_x = -1; offset_x <= 1; offset_x++) {
                    u8 neighbor = 0;
                    if (tiles.read_height(x + offset_x, y + offset_y,
                                          neighbor)) {
                        sum += neighbor;
                        count++;
                    }
                }
            }
            if (count == 0) return false;

            const f32 target = (f32)sum / (f32)count;
            return tiles.write_height(
                x, y, current + (i32)std::round((target - current) * amount),
                edit);
        }
        case TerrainBrush::Splat:
            return tiles.write_controlmap(x, y, amount,
                                          setting.terrain_palette_slot, edit);
        case TerrainBrush::Pick:
            return false;
    }
    return false;
}

}  // namespace

TerrainEdit apply_terrain_brush(TerrainTileMap &tiles, i32 x, i32 y,
                                TerrainBrush brush,
                                const TerrainBrushSetting &setting) {
    TerrainEdit edit;
    if (brush == TerrainBrush::Pick) return edit;

    const i32 radius = (i32)setting.radius;
    for (i32 offset_y = -radius; offset_y <= radius; offset_y++) {
        for (i32 offset_x = -radius; offset_x <= radius; offset_x++) {
            const f32 distance =
                std::sqrt((f32)(offset_x * offset_x + offset_y * offset_y));
            if (distance > (f32)radius) continue;

            const f32 falloff = 1.0f - distance / std::max(1.0f, (f32)radius);
            const f32 amount =
                std::clamp((f32)setting.strength * 0.01f * falloff, 0.0f, 1.0f);
            apply_brush_sample(tiles, x + offset_x, y + offset_y, brush, amount,
                               setting, edit);
        }
    }
    return edit;
}

}  // namespace Seed
