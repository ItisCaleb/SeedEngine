#ifndef _SEED_TERRAIN_TILE_MAP_H_
#define _SEED_TERRAIN_TILE_MAP_H_

#include <map>
#include <set>
#include <utility>

#include "core/ref.h"
#include "core/resource/image.h"
#include "core/types.h"

namespace Seed {

class EditorTile {
    public:
        enum class Direction { UP, BOTTOM, RIGHT, LEFT };
        enum class ImageType { Heightmap, Controlmap };

    private:
        static void clamp_border(Ref<Image> image);
        bool build_edge_from_image(Ref<Image> image, Ref<Image> source,
                                   Direction direction);
        bool build_border_from_image(Ref<Image> image, Ref<Image> source,
                                     Direction direction);

    public:
        i32 x = 0;
        i32 y = 0;
        Ref<Image> heightmap;
        Ref<Image> controlmap;

        Ref<Image> get_image(ImageType image_type) const;
        void clamp_border(ImageType image_type);
        bool build_edge_from_tile(EditorTile *tile, Direction direction);
        bool build_edge_from_tile(EditorTile *tile, Direction direction,
                                  ImageType image_type);
        bool build_border_from_tile(EditorTile *tile, Direction direction,
                                    ImageType image_type);
};

struct TerrainEdit {
        std::set<u32> heightmaps;
        std::set<u32> controlmaps;

        bool has_changes() const {
            return !heightmaps.empty() || !controlmaps.empty();
        }
};

class TerrainTileMap {
    private:
        u32 next_index = 0;
        std::map<std::pair<i32, i32>, u32> position_to_index;
        std::map<u32, EditorTile> tiles;
        std::set<u32> dirty_heightmaps;
        std::set<u32> dirty_controlmaps;

        i32 find_tile_index(i32 world_x, i32 world_y) const;
        bool world_to_pixel(i32 world_x, i32 world_y, u32 &tile_index,
                            u32 &pixel_x, u32 &pixel_y) const;
        void rebuild_border(u32 tile_index, EditorTile::ImageType image_type);
        void sync_neighbor(u32 tile_index, i32 neighbor_index,
                           EditorTile::Direction direction,
                           EditorTile::ImageType image_type,
                           std::set<u32> &changed_tiles);
        void sync_seams(std::set<u32> &changed_tiles,
                        EditorTile::ImageType image_type);

    public:
        u32 add_tile(i32 x, i32 y, Ref<Image> heightmap, Ref<Image> controlmap);
        void clear();

        i32 find_tile_index_at(i32 x, i32 y) const;
        bool tile_exists_at(i32 x, i32 y) const;
        EditorTile *get_tile_at(i32 x, i32 y);
        EditorTile *get_tile(u32 index);
        const EditorTile *get_tile(u32 index) const;

        bool read_height(i32 world_x, i32 world_y, u8 &height) const;
        bool write_height(i32 world_x, i32 world_y, i32 height,
                          TerrainEdit &edit);
        bool write_controlmap(i32 world_x, i32 world_y, f32 amount,
                              i32 palette_slot, TerrainEdit &edit);

        void commit(TerrainEdit &edit);
        TerrainEdit connect_tile(u32 tile_index);
        TerrainEdit synchronize_loaded_tiles();

        bool has_dirty_maps() const {
            return !dirty_heightmaps.empty() || !dirty_controlmaps.empty();
        }
        const std::set<u32> &get_dirty_heightmaps() const {
            return dirty_heightmaps;
        }
        const std::set<u32> &get_dirty_controlmaps() const {
            return dirty_controlmaps;
        }
        void clear_dirty_maps();
};

}  // namespace Seed

#endif
