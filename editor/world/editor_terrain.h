#ifndef _SEED_EDITOR_TERRAIN_H_
#define _SEED_EDITOR_TERRAIN_H_
#include <map>
#include <set>
#include <vector>
#include "core/ref.h"
#include "core/resource/image.h"
#include "core/resource/resource.h"
#include "core/resource/texture.h"
#include "core/resource/world_setting.h"
#include "core/world/terrain.h"

namespace Seed {

enum class TerrainBrush { Raise, Lower, Smooth, Flatten, Pick, Splat };
struct TerrainBrushSetting {
        u32 radius = 12;
        u32 strength = 25;
        u32 flatten_height = 128;
        i32 terrain_palette_slot = -1;
};

class EditorTile {
    public:
        enum class TileDirection { UP, BOTTOM, RIGHT, LEFT };
        enum class TileImage { Heightmap, Controlmap };

    private:
        void clamp_border(Ref<Image> image);
        bool build_edge_from_image(Ref<Image> image, Ref<Image> source,
                                   TileDirection direction);
        bool build_border_from_image(Ref<Image> image, Ref<Image> source,
                                     TileDirection direction);

    public:
        i32 x = 0;
        i32 y = 0;
        Ref<Image> heightmap;
        Ref<Image> controlmap;

        Ref<Image> get_image(TileImage image_type) const;
        void clamp_border();
        void clamp_border(TileImage image_type);
        bool build_edge_from_tile(EditorTile *tile, TileDirection direction);
        bool build_edge_from_tile(EditorTile *tile, TileDirection direction,
                                  TileImage image_type);
        bool build_border_from_tile(EditorTile *tile, TileDirection direction);
        bool build_border_from_tile(EditorTile *tile, TileDirection direction,
                                    TileImage image_type);
};

class TerrainEditor;
class EditorTerrain : public Resource {
        friend TerrainEditor;

    private:
        u32 last_heightmap = 0;
        Ref<Mesh> mesh;
        Ref<TextureArray> heightmaps;
        Ref<TextureArray> controlmaps;
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;
        Ref<Image> fallback_texture;
        Ref<Image> fallback_normal;

        Ref<TerrainMaterial> material;
        Ref<TerrainInstanceData> instances;

        std::map<std::pair<i32, i32>, u32> pos_to_index;
        std::map<u32, EditorTile> tiles;
        std::set<u32> dirty_heightmaps;
        std::set<u32> dirty_controlmaps;

        struct TerrainEdit {
                std::set<u32> heightmaps;
                std::set<u32> controlmaps;

                bool has_changes() const {
                    return !heightmaps.empty() || !controlmaps.empty();
                }
        };

        void build_mesh();
        void upload_fallback_layer(u32 layer);
        i32 find_chunk_index(i32 world_x, i32 world_y) const;
        bool world_to_heightmap_pixel(i32 world_x, i32 world_y, u32 &chunk_idx,
                                      u32 &pixel_x, u32 &pixel_y) const;
        void rebuild_tile_border(u32 chunk_index,
                                 EditorTile::TileImage image_type);
        void sync_tile_neighbor(u32 chunk_index, i32 neighbor_index,
                                EditorTile::TileDirection direction,
                                EditorTile::TileImage image_type,
                                std::set<u32> &touched_chunks);
        void sync_tile_seams(std::set<u32> &touched_chunks,
                             EditorTile::TileImage image_type);
        void commit_edit(TerrainEdit &edit);
        bool write_height(i32 world_x, i32 world_y, i32 height,
                          TerrainEdit &edit);
        bool write_controlmap(i32 world_x, i32 world_y, f32 amount,
                              i32 palette_slot, TerrainEdit &edit);
        void upload_heightmaps(const std::set<u32> &touched_chunks);
        void upload_controlmaps(const std::set<u32> &touched_chunks);
        bool apply_brush_sample(i32 x, i32 y, TerrainBrush type, f32 amount,
                                const TerrainBrushSetting &setting,
                                TerrainEdit &edit);

    public:
        EditorTerrain();
        void add_chunk(i32 x, i32 y, Ref<Image> height_map,
                       Ref<Image> control_map);
        void clear_chunks();
        void update_chunk_heightmap(u32 chunk_index, Ref<Image> height_map);
        void update_chunk_controlmap(u32 chunk_index, Ref<Image> control_map);
        void connect_chunk(u32 chunk_index);
        void sync_loaded_tile_seams();
        bool update_texture_layer(u32 layer, RHI::UpdateBufferInfo info);
        bool update_normal_layer(u32 layer, RHI::UpdateBufferInfo info);
        void reset_texture_palette();
        Ref<Mesh> get_mesh() { return mesh; }
        Ref<TerrainInstanceData> get_instances() { return instances; }
        i32 find_chunk_index_at(i32 x, i32 y) const;
        bool chunk_exists_at(i32 x, i32 y) const;
        EditorTile *get_tile_at(i32 x, i32 y);
        EditorTile *get_tile(u32 index);
        bool read_height(i32 world_x, i32 world_y, u8 &height);
        bool write_height(i32 world_x, i32 world_y, i32 height);
        bool write_controlmap(i32 world_x, i32 world_y, f32 amount,
                              i32 palette_slot);
        bool apply_brush(i32 x, i32 y, TerrainBrush type,
                         const TerrainBrushSetting &setting);
        bool has_dirty_maps() const {
            return !dirty_heightmaps.empty() || !dirty_controlmaps.empty();
        }
        void clear_dirty_maps();
        void save_dirty_maps(const std::vector<ChunkSetting> &chunks);
        ~EditorTerrain() = default;
};
}  // namespace Seed

#endif
