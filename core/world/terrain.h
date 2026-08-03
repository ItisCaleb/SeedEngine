#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/math/vec2.h"
#include "core/resource/material.h"
#include "core/resource/image.h"
#include "core/physic/physic_body.h"
#include "core/collision/shape.h"
#include <vector>
#include "core/rendering/mesh.h"
#include "core/rendering/instance_batch.h"
#include "core/resource/texture.h"

namespace Seed {

#define CHUNK_SIZE (256)
#define HEIGHTMAP_INNER_SIZE (CHUNK_SIZE + 1)
#define HEIGHTMAP_BORDER (1)
#define HEIGHTMAP_SIZE (HEIGHTMAP_INNER_SIZE + HEIGHTMAP_BORDER * 2)
#define HEIGHTMAP_INNER_FIRST (HEIGHTMAP_BORDER)
#define HEIGHT_OFFSET (-128)
#define HEIGHT_SCALE (1)
#define TERRAIN_CHUNK_LAYERS (256)
#define TERRAIN_TEXTURE_SIZE (1024)
#define TERRAIN_TEXTURE_LAYERS (16)

struct TerrainVertex {
        Vec2 pos;
};

struct TerrainInstance {
        Vec2 pos;
        u32 heightmap_index;
        f32 max_height, min_height;
};

class TerrainMaterial : public Material {
    private:
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;

    public:
        TerrainMaterial(Ref<Shader> shader, Ref<TextureArray> heightmaps,
                        Ref<TextureArray> controlmaps,
                        Ref<TextureArray> textures,
                        Ref<TextureArray> texture_normals);
        TerrainMaterial(Ref<TextureArray> heightmaps,
                        Ref<TextureArray> controlmaps,
                        Ref<TextureArray> textures,
                        Ref<TextureArray> texture_normals);
        Ref<TextureArray> get_textures() { return textures; }
        Ref<TextureArray> get_texture_normals() { return texture_normals; }
};

class TerrainInstanceBatch : public InstanceBase<TerrainInstanceBatch> {
    private:
        std::vector<TerrainInstance> instances;

    public:
        void insert_terrain_data(const TerrainInstance &instance);
        void update_height_range(u32 index, f32 min_height, f32 max_height);
        u32 size() override { return instances.size(); }
        void prepare_uploads(
            std::vector<RHI::UpdateBufferInfo> &uploads) override;
        AABB translate_bounding_box(const AABB &bounding_box, u32 i) override;
        void clear() override {
            instances.clear();
            mark_dirty();
        }
        TerrainInstanceBatch();
};

class Terrain : public RefCounted {
    private:
        u32 last_heightmap = 0;

        std::vector<PhysicBody> bodies;
        std::vector<Vec2> chunk_positions;
        Ref<TextureArray> heightmaps;
        Ref<TextureArray> controlmaps;
        Ref<TextureArray> textures;
        Ref<TextureArray> texture_normals;
        Ref<Image> fallback_texture;
        Ref<Image> fallback_normal;

        Ref<Material> material;
        Ref<Mesh> mesh;

        Ref<TerrainInstanceBatch> instances;
        void build_mesh();
        void upload_fallback_layer(u32 layer);

    public:
        explicit Terrain();
        static Ref<Image> create_default_heightmap();
        static Ref<Image> create_default_controlmap();
        Ref<Material> get_material() { return material; }
        void set_material(Ref<Material> material) {
            if (!material.is_null()) {
                this->material = material;
                this->mesh->set_material(material);
            }
        }
        Ref<TerrainInstanceBatch> get_instance() { return instances; }
        Ref<Mesh> get_mesh() { return mesh; }
        void add_chunk(i32 x, i32 y, Ref<Image> height_map,
                       Ref<Image> control_map);
        void clear_chunks();

        void update_heightmap_layer(u32 layer, Ref<Image> heightmap);
        void update_controlmap_layer(u32 layer, Ref<Image> controlmap);
        void refresh_chunk_collision(u32 layer, Ref<Image> heightmap);
        bool update_texture_layer(u32 layer, RHI::UpdateBufferInfo info);
        bool update_normal_layer(u32 layer, RHI::UpdateBufferInfo info);
        Ref<TextureArray> get_heightmaps() { return heightmaps; }
        Ref<TextureArray> get_controlmaps() { return controlmaps; }
        Ref<TextureArray> get_textures() { return textures; }
        Ref<TextureArray> get_normals() { return texture_normals; }

        void reset_texture_palette();

        ~Terrain();
};

}  // namespace Seed

#endif
