#include "editor_world.h"

#include <algorithm>

#include <fmt/format.h>

#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"

namespace Seed {

EditorWorld::EditorWorld(ResourceEntry *entry) : state(entry) {
    terrain.create();
    reload();
}

void EditorWorld::update_skybox_face(UUID uuid, CubemapFace face) {
    state.update_skybox_face(uuid, face);
}

bool EditorWorld::update_terrain_texture(u32 layer, UUID uuid,
                                         TerrainTextureKind kind) {
    if (setting.is_null() || terrain.is_null() || uuid.is_null() ||
        layer >= TERRAIN_TEXTURE_LAYERS || System::gResourceLoader == nullptr) {
        return false;
    }

    RHI::UpdateBufferInfo info =
        System::gResourceLoader->load_image_to_upload(uuid, true);
    if (info.data == nullptr) return false;

    if (kind == TerrainTextureKind::Diffuse) {
        if (!terrain->update_texture_layer(layer, info)) return false;
    } else if (!terrain->update_normal_layer(layer, info)) {
        return false;
    }

    const u32 palette_size = layer + 1;
    if (setting->terrain_textures.size() < palette_size) {
        setting->terrain_textures.resize(palette_size);
    }
    if (setting->terrain_normals.size() < palette_size) {
        setting->terrain_normals.resize(palette_size);
    }

    if (kind == TerrainTextureKind::Diffuse) {
        setting->terrain_textures[layer] = uuid;
        if (setting->terrain_normals[layer].is_null()) {
            terrain->update_normal_layer(layer, {});
        }
    } else {
        setting->terrain_normals[layer] = uuid;
        if (setting->terrain_textures[layer].is_null()) {
            terrain->update_texture_layer(layer, {});
        }
    }
    return true;
}

bool EditorWorld::terrain_chunk_exists_at(i32 x, i32 y) const {
    return !terrain.is_null() && terrain->chunk_exists_at(x, y);
}

bool EditorWorld::has_dirty_terrain_maps() const {
    return !terrain.is_null() && terrain->has_dirty_maps();
}

void EditorWorld::save_dirty_terrain_maps() {
    if (setting.is_null() || terrain.is_null()) return;
    terrain->save_dirty_maps(setting->chunks);
}

bool EditorWorld::add_new_chunk(i32 x, i32 y) {
    if (setting.is_null() || terrain.is_null() || System::gEditor == nullptr ||
        terrain_chunk_exists_at(x, y)) {
        return false;
    }

    ResourceEntry *heightmap_entry = System::gEditor->create_internal_asset(
        fmt::format("{}_{}_{}.png", setting->name, x, y), type_id<Texture>());
    if (heightmap_entry == nullptr) return false;

    ResourceEntry *controlmap_entry = System::gEditor->create_internal_asset(
        fmt::format("{}_{}_{}_control.png", setting->name, x, y),
        type_id<Texture>());
    if (controlmap_entry == nullptr) {
        System::gEditor->remove_asset(heightmap_entry->uuid);
        return false;
    }

    Ref<Image> heightmap;
    heightmap.create(PixelFormat::RG, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    heightmap->fill(Color{64, 64}, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);

    Ref<Image> controlmap;
    controlmap.create(PixelFormat::RGBA, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    controlmap->fill(Color{0, 0, 0, 0}, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);

    ChunkSetting chunk;
    chunk.x = x;
    chunk.y = y;
    chunk.height_map = heightmap_entry->uuid;
    chunk.control_map = controlmap_entry->uuid;
    setting->chunks.push_back(chunk);

    const u32 chunk_index = (u32)setting->chunks.size() - 1;
    terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
    terrain->connect_chunk(chunk_index);
    heightmap->save_disk(heightmap_entry->real_path());
    controlmap->save_disk(controlmap_entry->real_path());
    return true;
}

void EditorWorld::upload_terrain_palette_to_gpu() {
    if (setting.is_null() || terrain.is_null() ||
        System::gResourceLoader == nullptr) {
        return;
    }

    terrain->reset_texture_palette();
    const u32 palette_size =
        std::min(std::max((u32)setting->terrain_textures.size(),
                          (u32)setting->terrain_normals.size()),
                 (u32)TERRAIN_TEXTURE_LAYERS);
    for (u32 layer = 0; layer < palette_size; layer++) {
        RHI::UpdateBufferInfo texture_info{};
        if (layer < setting->terrain_textures.size() &&
            !setting->terrain_textures[layer].is_null()) {
            texture_info = System::gResourceLoader->load_image_to_upload(
                setting->terrain_textures[layer], true);
        }
        terrain->update_texture_layer(layer, texture_info);

        RHI::UpdateBufferInfo normal_info{};
        if (layer < setting->terrain_normals.size() &&
            !setting->terrain_normals[layer].is_null()) {
            normal_info = System::gResourceLoader->load_image_to_upload(
                setting->terrain_normals[layer], true);
        }
        terrain->update_normal_layer(layer, normal_info);
    }
}

void EditorWorld::clear_tiles() {
    if (setting.is_null() || terrain.is_null()) return;
    setting->chunks.clear();
    terrain->clear_chunks();
    rebuild_static_models();
}

void EditorWorld::reload() {
    terrain->clear_chunks();
    setting = state.load();

    upload_terrain_palette_to_gpu();
    state.load_sky(setting->sky);
    state.apply_directional_light(setting->dir_light);

    if (System::gResourceLoader != nullptr) {
        for (const ChunkSetting &chunk : setting->chunks) {
            Ref<Image> heightmap =
                System::gResourceLoader->load_image(chunk.height_map);
            Ref<Image> controlmap =
                System::gResourceLoader->load_image(chunk.control_map);
            terrain->add_chunk(chunk.x, chunk.y, heightmap, controlmap);
        }
    }

    terrain->sync_loaded_tile_seams();
    terrain->clear_dirty_maps();
    state.rebuild_static_models(setting->chunks);
}

void EditorWorld::save() {
    if (setting.is_null()) return;
    state.write_sky_setting(setting->sky);
    state.save(*setting.ptr());
}

void EditorWorld::rebuild_static_models() {
    if (setting.is_null()) return;
    state.rebuild_static_models(setting->chunks);
}

void EditorWorld::apply_directional_light() {
    if (setting.is_null()) return;
    state.apply_directional_light(setting->dir_light);
}

}  // namespace Seed
