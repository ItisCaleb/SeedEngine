#include "editor_session.h"

#include <fmt/format.h>

#include "core/misc/type_name.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/world/world.h"
#include "editor/editor.h"
#include "editor/project/project.h"

namespace Seed {

bool WorldSession::get_cubemap_face(Property property,
                                    CubemapFace &face) const {
    switch (property) {
        case Property::SkyUp:
            face = CubemapFace::TOP;
            return true;
        case Property::SkyDown:
            face = CubemapFace::BOTTOM;
            return true;
        case Property::SkyLeft:
            face = CubemapFace::LEFT;
            return true;
        case Property::SkyRight:
            face = CubemapFace::RIGHT;
            return true;
        case Property::SkyFront:
            face = CubemapFace::FRONT;
            return true;
        case Property::SkyBack:
            face = CubemapFace::BACK;
            return true;
        default:
            return false;
    }
}

bool WorldSession::is_texture_asset(UUID uuid) const {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    return !uuid.is_null() && entry != nullptr &&
           entry->type_id == type_id<Texture>();
}

void WorldSession::apply_directional_light() {
    World *world = System::gEditor->get_world();
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (world == nullptr || setting.is_null()) return;

    const DirectionalLightSetting &light = setting->dir_light;
    world->get_direction_light() =
        DirectionalLight(light.direction, light.diffuse, light.specular, true);
}

bool WorldSession::update_skybox_face(UUID uuid, CubemapFace face) {
    World *world = System::gEditor->get_world();
    if (world == nullptr || uuid.is_null() ||
        System::gResourceLoader == nullptr) {
        return false;
    }

    Ref<Sky> sky = world->get_sky();
    if (sky.is_null()) return false;

    Ref<TextureCubemap> cubemap = sky->get_cubemap();
    if (cubemap.is_null()) return false;

    Ref<Image> image = System::gResourceLoader->load_image(uuid, true);
    if (image.is_null()) return false;

    cubemap->update_face(cubemap->get_width(), cubemap->get_height(), face,
                         image);
    return true;
}

bool WorldSession::is_available() const { return System::gEditor->has_world(); }

void WorldSession::save() {
    status_text = System::gEditor->save_world() ? "World saved."
                                                : "Failed to save world.";
}

void WorldSession::build_inspector(InspectorBuilder &builder) {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (setting.is_null()) return;

    builder.begin_section("World");
    builder.text((i32)Property::Name, "Name", setting->name);

    builder.begin_section("Skybox");
    builder.resource((i32)Property::SkyUp, "Up", setting->sky.up);
    builder.resource((i32)Property::SkyDown, "Down", setting->sky.down);
    builder.resource((i32)Property::SkyLeft, "Left", setting->sky.left);
    builder.resource((i32)Property::SkyRight, "Right", setting->sky.right);
    builder.resource((i32)Property::SkyFront, "Front", setting->sky.front);
    builder.resource((i32)Property::SkyBack, "Back", setting->sky.back);

    builder.begin_section("Directional Light");
    builder.vec3((i32)Property::LightDirection, "Direction",
                 setting->dir_light.direction);
    builder.vec3((i32)Property::LightDiffuse, "Diffuse",
                 setting->dir_light.diffuse, InspectorVectorComponents::RGB);
    builder.vec3((i32)Property::LightSpecular, "Specular",
                 setting->dir_light.specular, InspectorVectorComponents::RGB);
}

bool WorldSession::commit_field(const InspectorField &field) {
    const Property property = (Property)field.id;
    if (property == Property::LightDirection ||
        property == Property::LightDiffuse ||
        property == Property::LightSpecular) {
        apply_directional_light();
    } else if (property != Property::Name) {
        CubemapFace face;
        if (!get_cubemap_face(property, face)) return false;
        if (!is_texture_asset(field.resource())) {
            status_text = "Skybox faces only accept texture assets.";
            return false;
        }
        if (!update_skybox_face(field.resource(), face)) {
            status_text = "Failed to update skybox face.";
            return false;
        }
    }

    status_text = System::gEditor->save_world()
                      ? "World settings updated and saved."
                      : "World settings updated but not saved.";
    return true;
}

KStr WorldSession::get_viewport_message() const {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (setting.is_null()) return "Open a world from Assets.";
    return setting->chunks.empty() ? KStr("Add a terrain tile.") : KStr("");
}

bool WorldSession::show_viewport_empty() const {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    return setting.is_null() || setting->chunks.empty();
}

void WorldSession::on_world_changed(bool loaded) {
    status_text = loaded ? KString("World loaded.") : KString();
}

bool WorldSession::handle_viewport_pick(const PickResult &,
                                        const EditorViewportInput &) {
    return false;
}

void WorldSession::handle_viewport_scroll(f32 delta) {
    System::gEditor->get_world_viewport().add_scroll(delta);
}

TerrainSession::TerrainSession() { brush_setting.terrain_palette_slot = 0; }

bool TerrainSession::is_texture_asset(UUID uuid) const {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    return !uuid.is_null() && entry != nullptr &&
           entry->type_id == type_id<Texture>();
}

bool TerrainSession::is_available() const {
    return System::gEditor->has_world();
}

void TerrainSession::save() {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (terrain.is_valid() && setting.is_valid()) {
        terrain->save_dirty_maps(setting->chunks);
    }
    status_text = System::gEditor->save_world() ? "World saved."
                                                : "Failed to save world.";
}

void TerrainSession::on_world_changed(bool loaded) {
    status_text = loaded ? KString("World loaded.") : KString();
    viewport_text = "-";
    terrain = {};
    brush_type = TerrainBrush::Raise;
    brush_setting = {};
    brush_setting.terrain_palette_slot = 0;

    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (!loaded || setting.is_null()) return;

    terrain.create();
    terrain->load_chunks(setting->chunks);
}

void TerrainSession::build_inspector(InspectorBuilder &builder) {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (setting.is_null()) return;

    builder.begin_section("Terrain");
    builder.read_only(
        (i32)Property::DirtyMaps, "Dirty maps",
        terrain.is_valid() && terrain->has_dirty_maps() ? "Yes" : "No");
    builder.action((i32)Action::AddTile, "Add Tile");
    builder.confirmation_action(
        (i32)Action::ClearTiles, "Clear", "Clear Terrain Tiles?",
        "This removes every terrain tile from the current world.", true);

    builder.begin_section("Tool");
    InspectorField &tools =
        builder.options((i32)Property::Tool, "Tool", brush_type);
    builder.option(tools, "Raise", (i32)TerrainBrush::Raise);
    builder.option(tools, "Lower", (i32)TerrainBrush::Lower);
    builder.option(tools, "Smooth", (i32)TerrainBrush::Smooth);
    builder.option(tools, "Flatten", (i32)TerrainBrush::Flatten);
    builder.option(tools, "Pick", (i32)TerrainBrush::Pick);
    builder.option(tools, "Splat", (i32)TerrainBrush::Splat);
    builder.range((i32)Property::Radius, "Radius", brush_setting.radius, 0, 50);
    builder.range((i32)Property::Strength, "Strength", brush_setting.strength,
                  0, 50);
    if (brush_type == TerrainBrush::Flatten) {
        builder.range((i32)Property::FlattenHeight, "Height",
                      brush_setting.flatten_height, 0, 255);
    }

    builder.begin_section("Texture Layers");
    InspectorField &palette = builder.options(
        (i32)Property::Palette, "Layer", brush_setting.terrain_palette_slot);
    for (i32 slot = 0; slot < TERRAIN_TEXTURE_LAYERS; slot++) {
        builder.option(palette, fmt::format("{}", slot), slot);
    }

    const i32 slot = brush_setting.terrain_palette_slot;
    if (slot < 0 || slot >= TERRAIN_TEXTURE_LAYERS) return;

    const u32 palette_size = (u32)slot + 1;
    if (setting->terrain_textures.size() < palette_size) {
        setting->terrain_textures.resize(palette_size);
    }
    if (setting->terrain_normals.size() < palette_size) {
        setting->terrain_normals.resize(palette_size);
    }
    builder.resource((i32)Property::Diffuse, "Diffuse",
                     setting->terrain_textures[(u32)slot]);
    builder.resource((i32)Property::Normal, "Normal",
                     setting->terrain_normals[(u32)slot]);
}

bool TerrainSession::commit_field(const InspectorField &field) {
    const Property property = (Property)field.id;
    if (property == Property::Diffuse || property == Property::Normal) {
        return update_terrain_texture(property, field.resource());
    }
    return true;
}

void TerrainSession::invoke_action(i32 action_id) {
    const Action action = (Action)action_id;
    if (action == Action::AddTile) {
        i32 chunk_x = 0;
        i32 chunk_y = 0;
        if (!add_chunk(chunk_x, chunk_y)) {
            status_text = "Failed to add terrain tile.";
        } else if (System::gEditor->save_world()) {
            status_text =
                fmt::format("Added terrain tile ({}, {}).", chunk_x, chunk_y);
        } else {
            status_text =
                "Terrain tile added, but the world could not be saved.";
        }
        return;
    }

    if (action != Action::ClearTiles) return;
    clear_chunks();
    status_text =
        System::gEditor->save_world()
            ? "All terrain tiles cleared."
            : "Terrain tiles cleared, but the world could not be saved.";
}

bool TerrainSession::chunk_exists_at(i32 chunk_x, i32 chunk_y) const {
    return terrain.is_valid() && terrain->chunk_exists_at(chunk_x, chunk_y);
}

bool TerrainSession::add_chunk_at(i32 chunk_x, i32 chunk_y) {
    Ref<Project> project = System::gEditor->get_project();
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (terrain.is_null() || project.is_null() || setting.is_null() ||
        setting->chunks.size() >= TERRAIN_CHUNK_LAYERS ||
        chunk_exists_at(chunk_x, chunk_y)) {
        return false;
    }

    ResourceEntry *heightmap_entry = project->create_internal_asset(
        fmt::format("{}_{}_{}.png", setting->name, chunk_x, chunk_y),
        type_id<Texture>());
    if (heightmap_entry == nullptr) return false;

    ResourceEntry *controlmap_entry = project->create_internal_asset(
        fmt::format("{}_{}_{}_control.png", setting->name, chunk_x, chunk_y),
        type_id<Texture>());
    if (controlmap_entry == nullptr) {
        project->remove_asset(heightmap_entry->uuid);
        return false;
    }

    Ref<Image> heightmap = Terrain::create_default_heightmap();
    Ref<Image> controlmap = Terrain::create_default_controlmap();

    ChunkSetting chunk;
    chunk.x = chunk_x;
    chunk.y = chunk_y;
    chunk.height_map = heightmap_entry->uuid;
    chunk.control_map = controlmap_entry->uuid;
    setting->chunks.push_back(chunk);

    const u32 chunk_index = (u32)setting->chunks.size() - 1;
    terrain->add_chunk(chunk_x, chunk_y, heightmap, controlmap);
    terrain->connect_chunk(chunk_index);
    heightmap->save_disk(heightmap_entry->real_path());
    controlmap->save_disk(controlmap_entry->real_path());
    return true;
}

bool TerrainSession::add_chunk(i32 &chunk_x, i32 &chunk_y) {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (setting.is_null()) return false;

    if (setting->chunks.empty()) {
        chunk_x = 0;
        chunk_y = 0;
        return add_chunk_at(chunk_x, chunk_y);
    }

    constexpr i32 directions[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    for (const ChunkSetting &chunk : setting->chunks) {
        for (const auto &direction : directions) {
            const i32 next_x = chunk.x + direction[0];
            const i32 next_y = chunk.y + direction[1];
            if (chunk_exists_at(next_x, next_y)) continue;

            chunk_x = next_x;
            chunk_y = next_y;
            return add_chunk_at(chunk_x, chunk_y);
        }
    }
    return false;
}

void TerrainSession::clear_chunks() {
    Ref<Project> project = System::gEditor->get_project();
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (terrain.is_null() || project.is_null() || setting.is_null()) return;

    for (const ChunkSetting &chunk : setting->chunks) {
        if (!chunk.height_map.is_null())
            project->remove_asset(chunk.height_map);
        if (!chunk.control_map.is_null()) {
            project->remove_asset(chunk.control_map);
        }
    }
    setting->chunks.clear();
    terrain->clear_chunks();
    System::gEditor->get_world_viewport().reset();
}

void TerrainSession::update_terrain_focus(i32 world_x, i32 world_y) {
    if (terrain.is_null()) return;

    u8 height = 0;
    if (!terrain->read_height(world_x, world_y, height)) {
        System::gEditor->get_world_viewport().clear_camera_focus();
        return;
    }
    System::gEditor->get_world_viewport().set_camera_focus(
        Vec3{(f32)world_x, (f32)height + HEIGHT_OFFSET, (f32)world_y});
}

bool TerrainSession::update_terrain_texture(Property property, UUID uuid) {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    const i32 slot = brush_setting.terrain_palette_slot;
    if (terrain.is_null() || setting.is_null() || slot < 0 ||
        slot >= TERRAIN_TEXTURE_LAYERS) {
        return false;
    }

    if (!is_texture_asset(uuid)) {
        status_text = "Terrain layers only accept texture assets.";
        return false;
    }

    RHI::UpdateBufferInfo info =
        System::gResourceLoader->load_image_to_upload(uuid, true);
    if (info.data == nullptr) {
        status_text = "Failed to upload terrain texture.";
        return false;
    }

    const bool diffuse = property == Property::Diffuse;
    const bool updated = diffuse
                             ? terrain->update_texture_layer((u32)slot, info)
                             : terrain->update_normal_layer((u32)slot, info);
    if (!updated) {
        status_text = "Failed to upload terrain texture.";
        return false;
    }

    const u32 palette_size = (u32)slot + 1;
    if (setting->terrain_textures.size() < palette_size) {
        setting->terrain_textures.resize(palette_size);
    }
    if (setting->terrain_normals.size() < palette_size) {
        setting->terrain_normals.resize(palette_size);
    }

    if (diffuse) {
        if (setting->terrain_normals[(u32)slot].is_null()) {
            terrain->update_normal_layer((u32)slot, {});
        }
    } else {
        if (setting->terrain_textures[(u32)slot].is_null()) {
            terrain->update_texture_layer((u32)slot, {});
        }
    }

    const char *kind_name = diffuse ? "Diffuse" : "Normal";
    const bool saved = System::gEditor->save_world();
    status_text = fmt::format("Terrain layer {} {} texture updated {}.", slot,
                              kind_name, saved ? "and saved" : "but not saved");
    return true;
}

KStr TerrainSession::get_viewport_message() const {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    if (setting.is_null()) return "Open a world from Assets.";
    return setting->chunks.empty() ? KStr("Add a terrain tile.") : KStr("");
}

bool TerrainSession::show_viewport_empty() const {
    Ref<WorldSetting> setting = System::gEditor->get_world_setting();
    return setting.is_null() || setting->chunks.empty();
}

bool TerrainSession::handle_viewport_pick(const PickResult &result,
                                          const EditorViewportInput &input) {
    if (!input.valid || result.data[2] == 0) {
        viewport_text = "-";
        System::gEditor->get_world_viewport().clear_camera_focus();
        return false;
    }

    const i32 world_x = (i16)result.data[0];
    const i32 world_y = (i16)result.data[1];
    viewport_text = fmt::format("{}, {}", world_x, world_y);
    update_terrain_focus(world_x, world_y);

    if (!input.primary_pressed || input.alt_pressed ||
        brush_type == TerrainBrush::Pick || terrain.is_null()) {
        return false;
    }

    const bool changed =
        terrain->apply_brush(world_x, world_y, brush_type, brush_setting);
    if (changed) update_terrain_focus(world_x, world_y);
    return changed;
}

void TerrainSession::handle_viewport_scroll(f32 delta) {
    System::gEditor->get_world_viewport().add_scroll(delta);
}

}  // namespace Seed
