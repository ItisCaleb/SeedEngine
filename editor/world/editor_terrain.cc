#include "editor_terrain.h"

#include <algorithm>

#include "core/resource/default_storage.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/system.h"
#include "editor/editor_storage.h"
#include "core/world/world.h"

namespace Seed {

EditorTerrain::EditorTerrain() {
    terrain = System::gEngine->get_world()->get_terrain();
    Ref<Material> mat;
    mat.create(System::gEditorStorage->editor_terrain_shader,
               RenderRasterizerState{.cull_mode = Cullmode::BACK,
                                     .patch_control_points = 4},
               RenderDepthStencilState{.depth_mode = DepthMode::OPAQUE},
               RenderBlendState{});
    mat->set_texture("height_map",
                     ref_cast<Texture>(terrain->get_heightmaps()));
    mat->set_texture("control_map",
                     ref_cast<Texture>(terrain->get_controlmaps()));
    mat->set_texture("textures", ref_cast<Texture>(terrain->get_textures()));
    mat->set_texture("texture_normals",
                     ref_cast<Texture>(terrain->get_normals()));
    mat->set_texture("noise_texture", System::gDefaultStorage->noise_texture);
    terrain->set_material(mat);
}

void EditorTerrain::load_chunks(const std::vector<ChunkSetting> &chunks) {
    tile_map.clear();
    if (System::gResourceLoader == nullptr) return;
    ResourceLoader &loader = *System::gResourceLoader;

    const u32 chunk_count =
        std::min((u32)chunks.size(), (u32)TERRAIN_CHUNK_LAYERS);
    for (u32 chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
        const ChunkSetting &chunk = chunks[chunk_index];
        Ref<Image> heightmap = loader.load_image(chunk.height_map);
        Ref<Image> controlmap = loader.load_image(chunk.control_map);
        if (heightmap.is_null()) {
            heightmap = Terrain::create_default_heightmap();
        }
        if (controlmap.is_null()) {
            controlmap = Terrain::create_default_controlmap();
        }
        tile_map.add_tile(chunk.x, chunk.y, heightmap, controlmap);
    }

    sync_loaded_tile_seams();
    clear_dirty_maps();
}

void EditorTerrain::add_chunk(i32 x, i32 y, Ref<Image> heightmap,
                              Ref<Image> controlmap) {
    tile_map.add_tile(x, y, heightmap, controlmap);
    terrain->add_chunk(x, y, heightmap, controlmap);
}

void EditorTerrain::clear_chunks() {
    tile_map.clear();
    terrain->clear_chunks();
}

bool EditorTerrain::update_texture_layer(u32 layer,
                                         RHI::UpdateBufferInfo info) {
    return terrain->update_texture_layer(layer, info);
}

bool EditorTerrain::update_normal_layer(u32 layer, RHI::UpdateBufferInfo info) {
    return terrain->update_normal_layer(layer, info);
}

bool EditorTerrain::chunk_exists_at(i32 x, i32 y) const {
    return tile_map.tile_exists_at(x, y);
}

bool EditorTerrain::read_height(i32 world_x, i32 world_y, u8 &height) {
    return tile_map.read_height(world_x, world_y, height);
}

bool EditorTerrain::apply_brush(i32 x, i32 y, TerrainBrush brush,
                                const TerrainBrushSetting &setting) {
    TerrainEdit edit = apply_terrain_brush(tile_map, x, y, brush, setting);
    if (!edit.has_changes()) return false;

    commit_edit(edit);
    return true;
}

void EditorTerrain::commit_edit(TerrainEdit &edit) {
    tile_map.commit(edit);
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::connect_chunk(u32 chunk_index) {
    TerrainEdit edit = tile_map.connect_tile(chunk_index);
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::sync_loaded_tile_seams() {
    TerrainEdit edit = tile_map.synchronize_loaded_tiles();
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::upload_heightmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_index : touched_chunks) {
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (tile == nullptr || tile->heightmap.is_null()) continue;
        terrain->update_heightmap_layer(chunk_index, tile->heightmap);
    }
}

void EditorTerrain::upload_controlmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_index : touched_chunks) {
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (tile == nullptr || tile->controlmap.is_null()) continue;
        terrain->update_controlmap_layer(chunk_index, tile->controlmap);
    }
}

void EditorTerrain::clear_dirty_maps() { tile_map.clear_dirty_maps(); }

void EditorTerrain::save_dirty_maps(const std::vector<ChunkSetting> &chunks) {
    if (!tile_map.has_dirty_maps() || System::gResourceLoader == nullptr) {
        return;
    }

    for (u32 chunk_index : tile_map.get_dirty_heightmaps()) {
        if (chunk_index >= chunks.size()) continue;

        ResourceEntry *entry =
            System::gResourceEntries->get_entry(chunks[chunk_index].height_map);
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (entry == nullptr || tile == nullptr || tile->heightmap.is_null()) {
            continue;
        }
        tile->heightmap->save_disk(entry->real_path());
        terrain->refresh_chunk_collision(chunk_index, tile->heightmap);
    }

    for (u32 chunk_index : tile_map.get_dirty_controlmaps()) {
        if (chunk_index >= chunks.size()) continue;

        ResourceEntry *entry = System::gResourceEntries->get_entry(
            chunks[chunk_index].control_map);
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (entry == nullptr || tile == nullptr || tile->controlmap.is_null()) {
            continue;
        }
        tile->controlmap->save_disk(entry->real_path());
    }

    clear_dirty_maps();
}

}  // namespace Seed
