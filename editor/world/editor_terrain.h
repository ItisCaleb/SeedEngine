#ifndef _SEED_EDITOR_TERRAIN_H_
#define _SEED_EDITOR_TERRAIN_H_
#include "core/ref.h"
#include "core/resource/terrain.h"
#include "core/resource/texture.h"

namespace Seed {
class EditorTerrainMaterial : public Material {
    public:
        EditorTerrainMaterial(Ref<Texture> height_map, Ref<Texture> splat_map);
        Ref<Texture> get_height_map();
        void set_light_map(Ref<Texture> texture);
};

class TerrainEditor;
class EditorTerrain : public Resource {
        friend TerrainEditor;

    private:
        std::string name;
        u32 width, height;
        u32 hmap_width, hmap_height;
        Ref<Mesh> mesh;
        Ref<MappableTexture> heightmap_texture;
        Ref<MappableTexture> light_map;
        Ref<MappableTexture> splat_map;
        Ref<EditorTerrainMaterial> material;
        Ref<TerrainInstanceData> instances;
        bool light_map_generated = false;
        void build_mesh();
        void create_chunk(Ref<MappableTexture> height_map, i32 left, i32 bottom,
                          u32 half_width, u32 half_depth);
        void gen_lightmap();

    public:
        EditorTerrain(u32 width, u32 height, Ref<MappableTexture> height_map,
                      Ref<MappableTexture> splat_map,
                      Ref<MappableTexture> light_map);
        Ref<MappableTexture> get_heightmap() { return heightmap_texture; }
        Ref<MappableTexture> get_splatmap() { return splat_map; }
        Ref<EditorTerrainMaterial> get_material() { return material;}
        Ref<Mesh> get_mesh() { return mesh; }
        Ref<TerrainInstanceData> get_instances() { return instances; }
        u32 get_width() { return width; }
        u32 get_height() { return height; }
        void dump(const Path &dir);
        ~EditorTerrain();
};
}  // namespace Seed

#endif