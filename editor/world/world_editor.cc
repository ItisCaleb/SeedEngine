#include "world_editor.h"

#include <utility>

#include <fmt/format.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/input.h"
#include "core/misc/enums.h"
#include "core/misc/type_name.h"
#include "core/project.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/resource/texture.h"
#include "editor/editor.h"
#include "editor/world/editor_world.h"
#include "editor/world/world_renderer.h"

namespace Seed {

namespace {

constexpr u32 kViewportWidth = 1024;
constexpr u32 kViewportHeight = 768;

struct SkyboxFaceBinding {
        const char *name;
        UUID SkySetting::*setting;
        CubemapFace cubemap_face;
};

constexpr SkyboxFaceBinding skybox_faces[] = {
    {"up", &SkySetting::up, CubemapFace::TOP},
    {"down", &SkySetting::down, CubemapFace::BOTTOM},
    {"left", &SkySetting::left, CubemapFace::LEFT},
    {"right", &SkySetting::right, CubemapFace::RIGHT},
    {"front", &SkySetting::front, CubemapFace::FRONT},
    {"back", &SkySetting::back, CubemapFace::BACK},
};

const SkyboxFaceBinding *find_skybox_face(const Rml::String &name) {
    for (const SkyboxFaceBinding &face : skybox_faces) {
        if (name == face.name) return &face;
    }
    return nullptr;
}

bool is_texture_asset(UUID uuid) {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    return !uuid.is_null() && entry != nullptr &&
           entry->type_id == type_id<Texture>();
}

template <typename T>
void register_enum(Rml::DataModelConstructor &constructor) {
    constructor.RegisterScalar<T>(
        [](const T &value, Rml::Variant &variant) {
            variant = Rml::String(enum_to_string(value));
        },
        [](T &value, const Rml::Variant &variant) {
            value = string_to_enum<T>(variant.Get<Rml::String>());
        });
}

}  // namespace

WorldEditor::~WorldEditor() {
    world_setting = &empty_world_setting;
    delete current_world;
}

bool WorldEditor::load_world(const UUID uuid) {
    set_status("");

    world_setting = &empty_world_setting;
    delete current_world;
    current_world = nullptr;
    reset_selection();

    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (entry == nullptr) {
        set_status("World file is not registered in resource entries.");
        sync_view_model();
        dirty_view_model();
        return false;
    }

    current_world = new EditorWorld(entry);
    world_setting = current_world->get_setting();
    if (System::gEditor != nullptr) {
        System::gEditor->set_last_open_world(entry->uuid);
    }
    set_status("World loaded.");
    sync_view_model();
    dirty_view_model();
    return true;
}

void WorldEditor::init() {
    renderer = new WorldRenderer(kViewportWidth, kViewportHeight);
    System::gGuiEngine->add_texture("main_view",
                                    renderer->get_screen_texture());
    System::gRenderEngine->register_renderer(1, renderer);
}

bool WorldEditor::is_viewport_hovered() const {
    if (document == nullptr) return false;

    Rml::Context *context = System::gGuiEngine->get_rml_context();
    Rml::Element *viewport = document->GetElementById("viewport");
    Rml::Element *hovered =
        context == nullptr ? nullptr : context->GetHoverElement();
    while (hovered != nullptr) {
        if (hovered == viewport) return true;
        hovered = hovered->GetParentNode();
    }
    return false;
}

bool WorldEditor::get_camera_focus(Vec3 &target) const {
    if (current_world == nullptr) return false;

    const std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (selected_static_chunk >= 0 && selected_static_object >= 0 &&
        selected_static_chunk < (i32)chunks.size()) {
        const ChunkSetting &chunk = chunks[(u32)selected_static_chunk];
        if (selected_static_object < (i32)chunk.static_objects.size()) {
            const StaticObjectSetting &object =
                chunk.static_objects[(u32)selected_static_object];
            target = Vec3{(f32)object.x, (f32)object.y, (f32)object.z};
            return true;
        }
    }

    if (!last_pick_valid || current_world->terrain.is_null()) return false;
    u8 height = 0;
    if (!current_world->terrain->read_height(last_pick_x, last_pick_y,
                                             height)) {
        return false;
    }
    target =
        Vec3{(f32)last_pick_x, (f32)height + HEIGHT_OFFSET, (f32)last_pick_y};
    return true;
}

f32 WorldEditor::consume_viewport_scroll() {
    const f32 delta = viewport_scroll_delta;
    viewport_scroll_delta = 0.0f;
    return delta;
}

bool WorldEditor::chunk_exists_at(i32 chunk_x, i32 chunk_y) const {
    return current_world != nullptr &&
           current_world->terrain_chunk_exists_at(chunk_x, chunk_y);
}

KString WorldEditor::static_model_label(UUID uuid) const {
    ResourceEntry *entry = System::gResourceEntries->get_entry(uuid);
    if (entry == nullptr) return KString("Missing Model");
    return KString(entry->path.filename());
}

void WorldEditor::select_static_object(u32 chunk_index, u32 object_index) {
    if (current_world == nullptr) return;
    selected_static_chunk = (i32)chunk_index;
    selected_static_object = (i32)object_index;
}

bool WorldEditor::save_current_world() {
    if (current_world == nullptr) return false;
    current_world->save_dirty_terrain_maps();
    current_world->save();

    Project *project = System::gEngine->get_project();
    if (project == nullptr) return false;

    System::gResourceEntries->save(project->get_entry_path());
    return true;
}

bool WorldEditor::add_chunk_at(i32 chunk_x, i32 chunk_y) {
    if (current_world == nullptr) return false;
    if (chunk_exists_at(chunk_x, chunk_y)) {
        set_status("Terrain tile already exists.");
        return false;
    }

    if (!current_world->add_new_chunk(chunk_x, chunk_y)) {
        set_status("Failed to add terrain tile.");
        return false;
    }

    if (!save_current_world()) {
        set_status("Terrain tile added, but the world could not be saved.");
        return true;
    }
    set_status(fmt::format("Added terrain tile ({}, {}).", chunk_x, chunk_y));
    return true;
}

void WorldEditor::add_chunk() {
    if (current_world == nullptr) return;
    const std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (chunks.empty()) {
        add_chunk_at(0, 0);
        return;
    }

    static const i32 dirs[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    for (const ChunkSetting &chunk : chunks) {
        for (u32 i = 0; i < 4; i++) {
            i32 next_x = chunk.x + dirs[i][0];
            i32 next_y = chunk.y + dirs[i][1];
            if (!chunk_exists_at(next_x, next_y)) {
                add_chunk_at(next_x, next_y);
                return;
            }
        }
    }
}

void WorldEditor::clear_tiles() {
    if (current_world == nullptr || System::gEditor == nullptr) return;

    std::vector<UUID> tile_assets;
    tile_assets.reserve(current_world->get_chunks().size() * 2);
    for (const ChunkSetting &chunk : current_world->get_chunks()) {
        if (!chunk.height_map.is_null()) {
            tile_assets.push_back(chunk.height_map);
        }
        if (!chunk.control_map.is_null()) {
            tile_assets.push_back(chunk.control_map);
        }
    }

    for (UUID uuid : tile_assets) {
        System::gEditor->remove_asset(uuid);
    }

    current_world->clear_tiles();
    reset_selection();
    if (save_current_world()) {
        set_status("All terrain tiles cleared.");
    } else {
        set_status("Terrain tiles cleared, but the world could not be saved.");
    }
}

void WorldEditor::set_status(std::string status) {
    status_text = std::move(status);
    has_status = !status_text.empty();
    if (!view_model) return;
    view_model.DirtyVariable("status");
    view_model.DirtyVariable("has_status");
}

void WorldEditor::sync_dirty_maps() {
    dirty_maps_text =
        current_world != nullptr && current_world->has_dirty_terrain_maps()
            ? "Yes"
            : "No";
}

void WorldEditor::reset_selection() {
    selected_static_chunk = -1;
    selected_static_object = -1;
    last_pick_valid = false;
    world_text = "-";
}

void WorldEditor::sync_scene_view_model() {
    scene_objects.clear();
    if (current_world == nullptr) {
        show_scene_empty = true;
        return;
    }

    const std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    for (u32 chunk_index = 0; chunk_index < chunks.size(); chunk_index++) {
        const ChunkSetting &chunk = chunks[chunk_index];
        for (u32 object_index = 0; object_index < chunk.static_objects.size();
             object_index++) {
            const StaticObjectSetting &object =
                chunk.static_objects[object_index];
            SceneObjectView view;
            view.asset = static_model_label(object.model);
            view.name = object.name.is_empty() ? view.asset : object.name;
            view.selected = selected_static_chunk == (i32)chunk_index &&
                            selected_static_object == (i32)object_index;
            view.chunk_index = (i32)chunk_index;
            view.object_index = (i32)object_index;
            scene_objects.push_back(view);
        }
    }

    show_scene_empty = scene_objects.empty();
}

void WorldEditor::sync_selected_object_view_model() {
    has_selected_object = false;
    selected_name.clear();
    selected_x = 0;
    selected_y = 0;
    selected_z = 0;
    selected_asset.clear();

    if (current_world == nullptr || selected_static_chunk < 0 ||
        selected_static_object < 0) {
        return;
    }

    std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (selected_static_chunk >= (i32)chunks.size()) return;

    ChunkSetting &chunk = chunks[(u32)selected_static_chunk];
    if (selected_static_object >= (i32)chunk.static_objects.size()) return;

    StaticObjectSetting &object =
        chunk.static_objects[(u32)selected_static_object];
    selected_name = object.name;
    selected_x = object.x;
    selected_y = object.y;
    selected_z = object.z;
    selected_asset = static_model_label(object.model);
    has_selected_object = true;
}

void WorldEditor::sync_terrain_palette_view_model() {
    terrain_palette.resize(TERRAIN_TEXTURE_LAYERS);
    for (u32 slot = 0; slot < TERRAIN_TEXTURE_LAYERS; slot++) {
        TerrainPaletteView &view = terrain_palette[slot];
        view.slot = (i32)slot;
        view.selected = brush_setting.terrain_palette_slot == (i32)slot;
    }

    const i32 slot = brush_setting.terrain_palette_slot;
    if (slot < 0 || slot >= TERRAIN_TEXTURE_LAYERS) {
        selected_terrain_diffuse = {};
        selected_terrain_normal = {};
        return;
    }

    selected_terrain_diffuse =
        (u32)slot < world_setting->terrain_textures.size()
            ? world_setting->terrain_textures[(u32)slot]
            : UUID{};
    selected_terrain_normal = (u32)slot < world_setting->terrain_normals.size()
                                  ? world_setting->terrain_normals[(u32)slot]
                                  : UUID{};
}

void WorldEditor::sync_view_model() {
    has_world = current_world != nullptr;
    has_status = !status_text.empty();
    sync_dirty_maps();
    if (current_world == nullptr) {
        viewport_message = "Open a world from Assets.";
        world_text = "-";
        show_viewport_empty = true;
        sync_scene_view_model();
        sync_selected_object_view_model();
        sync_terrain_palette_view_model();
        return;
    }

    show_viewport_empty = current_world->get_chunks().empty();
    viewport_message = show_viewport_empty ? "Add a terrain tile." : "";
    if (!last_pick_valid) world_text = "-";

    sync_scene_view_model();
    sync_selected_object_view_model();
    sync_terrain_palette_view_model();
}

void WorldEditor::dirty_view_model() {
    if (!view_model) return;
    view_model.DirtyAllVariables();
}

bool WorldEditor::viewport_event_to_pixel(Rml::Event &event, i32 &image_x,
                                          i32 &image_y) const {
    if (renderer == nullptr) return false;

    Rml::Element *element = event.GetCurrentElement();
    if (element == nullptr) return false;

    Ref<Texture> viewport_texture = renderer->get_screen_texture();
    if (viewport_texture.is_null()) return false;
    const u32 texture_width = viewport_texture->get_width();
    const u32 texture_height = viewport_texture->get_height();

    f32 x = event.GetParameter<f32>("mouse_x", -1.0f);
    f32 y = event.GetParameter<f32>("mouse_y", -1.0f);
    if (x < 0.0f || y < 0.0f) return false;

    Rml::Vector2f offset = element->GetAbsoluteOffset();
    Rml::Vector2f size = element->GetRenderBox().GetFillSize();
    if (size.x <= 0.0f || size.y <= 0.0f) return false;

    f32 local_x = x - offset.x;
    f32 local_y = y - offset.y;
    if (local_x < 0.0f || local_y < 0.0f || local_x >= size.x ||
        local_y >= size.y) {
        return false;
    }

    f32 pixel_x = (local_x / size.x) * (f32)texture_width;
    f32 pixel_y = (local_y / size.y) * (f32)texture_height;
    if (pixel_x < 0.0f || pixel_y < 0.0f || pixel_x >= texture_width ||
        pixel_y >= texture_height) {
        return false;
    }

    image_x = (i32)pixel_x;
    image_y = (i32)((f32)texture_height - 1.0f - pixel_y);
    return true;
}

bool WorldEditor::pick_world_at_pixel(i32 image_x, i32 image_y, i32 &world_x,
                                      i32 &world_y) const {
    if (renderer == nullptr) return false;

    Ref<MappableTexture> picking_texture = renderer->get_picking_texture();
    if (picking_texture.is_null()) return false;
    if (image_x < 0 || image_y < 0 ||
        image_x >= (i32)picking_texture->get_width() ||
        image_y >= (i32)picking_texture->get_height()) {
        return false;
    }

    i16 *pixel = (i16 *)picking_texture->pixel((u32)image_x, (u32)image_y);
    if (pixel[2] == 0) return false;

    world_x = pixel[0];
    world_y = pixel[1];
    return true;
}

bool WorldEditor::update_pick_from_event(Rml::Event &event) {
    i32 image_x = 0;
    i32 image_y = 0;
    i32 world_x = 0;
    i32 world_y = 0;
    if (!viewport_event_to_pixel(event, image_x, image_y) ||
        !pick_world_at_pixel(image_x, image_y, world_x, world_y)) {
        last_pick_valid = false;
        world_text = "-";
        return false;
    }

    last_pick_x = world_x;
    last_pick_y = world_y;
    last_pick_valid = true;
    world_text = fmt::format("{}, {}", world_x, world_y);
    return true;
}

void WorldEditor::rml_set_mode(RML_EVENT_ARGS) {
    if (args.empty()) return;
    active_mode =
        string_to_enum<WorldEditorMode>(args[0].Get<Rml::String>("World"));
    if (active_mode == WorldEditorMode::Terrain &&
        brush_setting.terrain_palette_slot < 0) {
        brush_setting.terrain_palette_slot = 0;
        sync_terrain_palette_view_model();
    }
    if (view_model) {
        view_model.DirtyVariable("editor_mode");
        view_model.DirtyVariable("brush_setting");
        view_model.DirtyVariable("terrain_palette");
        view_model.DirtyVariable("selected_terrain_diffuse");
        view_model.DirtyVariable("selected_terrain_normal");
    }
}

void WorldEditor::rml_set_tool(RML_EVENT_ARGS) {
    if (args.empty()) return;
    brush_type =
        string_to_enum<TerrainBrush>(args[0].Get<Rml::String>("Raise"));
    if (brush_type == TerrainBrush::Splat &&
        brush_setting.terrain_palette_slot < 0) {
        brush_setting.terrain_palette_slot = 0;
        sync_terrain_palette_view_model();
        if (view_model) {
            view_model.DirtyVariable("brush_setting");
            view_model.DirtyVariable("terrain_palette");
            view_model.DirtyVariable("selected_terrain_diffuse");
            view_model.DirtyVariable("selected_terrain_normal");
        }
    }
    if (view_model) view_model.DirtyVariable("terrain_tool");
}

void WorldEditor::rml_save_world(RML_EVENT_ARGS) {
    set_status(save_current_world() ? "World saved." : "Failed to save world.");
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_add_tile(RML_EVENT_ARGS) {
    add_chunk();
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_request_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = true;
    if (view_model) view_model.DirtyVariable("show_clear_tiles");
}

void WorldEditor::rml_cancel_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = false;
    if (view_model) view_model.DirtyVariable("show_clear_tiles");
}

void WorldEditor::rml_confirm_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = false;
    clear_tiles();
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_commit_world_settings(RML_EVENT_ARGS) {
    if (current_world == nullptr) return;

    current_world->apply_directional_light_to_runtime();
    set_status("World settings updated.");
}

void WorldEditor::rml_set_skybox_face(RML_EVENT_ARGS) {
    if (current_world == nullptr || args.empty()) return;

    const Rml::String face_name = args[0].Get<Rml::String>("");
    const SkyboxFaceBinding *face = find_skybox_face(face_name);
    if (face == nullptr) return;

    const UUID uuid = UUID::from_string(e.GetParameter<Rml::String>("value", ""));
    if (!is_texture_asset(uuid)) {
        world_setting->sky.*(face->setting) =
            current_world->get_sky().*(face->setting);
        set_status("Skybox faces only accept texture assets.");
        if (view_model) view_model.DirtyVariable("world");
        return;
    }

    EditorSky &sky = current_world->get_sky();
    sky.*(face->setting) = uuid;
    world_setting->sky.*(face->setting) = uuid;
    current_world->update_skybox_face(uuid, face->cubemap_face);
    if (save_current_world()) {
        set_status(fmt::format("Skybox {} face updated and saved.", face_name));
    } else {
        set_status(
            fmt::format("Skybox {} face updated but not saved.", face_name));
    }
    sync_dirty_maps();
    if (view_model) {
        view_model.DirtyVariable("world");
        view_model.DirtyVariable("dirty_maps");
    }
}

void WorldEditor::rml_select_terrain_layer(RML_EVENT_ARGS) {
    if (args.empty()) return;
    i32 slot = args[0].Get<i32>(-1);
    if (slot < 0 || slot >= TERRAIN_TEXTURE_LAYERS) return;

    brush_setting.terrain_palette_slot = slot;
    sync_terrain_palette_view_model();
    if (view_model) {
        view_model.DirtyVariable("brush_setting");
        view_model.DirtyVariable("terrain_palette");
        view_model.DirtyVariable("selected_terrain_diffuse");
        view_model.DirtyVariable("selected_terrain_normal");
    }
}

void WorldEditor::rml_set_terrain_texture(RML_EVENT_ARGS) {
    if (current_world == nullptr || args.empty()) return;

    const i32 slot = brush_setting.terrain_palette_slot;
    if (slot < 0 || slot >= TERRAIN_TEXTURE_LAYERS) return;

    const Rml::String kind_name = args[0].Get<Rml::String>("");
    TerrainTextureKind kind;
    if (kind_name == "Diffuse") {
        kind = TerrainTextureKind::Diffuse;
    } else if (kind_name == "Normal") {
        kind = TerrainTextureKind::Normal;
    } else {
        return;
    }

    UUID uuid = UUID::from_string(e.GetParameter<Rml::String>("value", ""));
    if (!is_texture_asset(uuid)) {
        set_status("Terrain layers only accept texture assets.");
        sync_terrain_palette_view_model();
        if (view_model) {
            view_model.DirtyVariable("selected_terrain_diffuse");
            view_model.DirtyVariable("selected_terrain_normal");
        }
        return;
    }

    if (!current_world->update_terrain_texture((u32)slot, uuid, kind)) {
        set_status("Failed to upload terrain texture.");
        sync_terrain_palette_view_model();
        if (view_model) {
            view_model.DirtyVariable("selected_terrain_diffuse");
            view_model.DirtyVariable("selected_terrain_normal");
        }
        return;
    }

    brush_setting.terrain_palette_slot = slot;
    bool saved = save_current_world();
    set_status(fmt::format("Terrain layer {} {} texture updated {}.", slot,
                           kind_name, saved ? "and saved" : "but not saved"));
    sync_terrain_palette_view_model();
    if (view_model) {
        view_model.DirtyVariable("brush_setting");
        view_model.DirtyVariable("terrain_palette");
        view_model.DirtyVariable("selected_terrain_diffuse");
        view_model.DirtyVariable("selected_terrain_normal");
    }
}

void WorldEditor::rml_select_scene_object(RML_EVENT_ARGS) {
    i32 index = args.empty() ? -1 : args[0].Get<int>(-1);
    if (index < 0 || index >= (i32)scene_objects.size()) return;

    SceneObjectView &object = scene_objects[(u32)index];
    select_static_object((u32)object.chunk_index, (u32)object.object_index);
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_commit_selected_object(RML_EVENT_ARGS) {
    if (current_world == nullptr || selected_static_chunk < 0 ||
        selected_static_object < 0) {
        return;
    }

    std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (selected_static_chunk >= (i32)chunks.size()) return;

    ChunkSetting &chunk = chunks[(u32)selected_static_chunk];
    if (selected_static_object >= (i32)chunk.static_objects.size()) return;

    StaticObjectSetting &object =
        chunk.static_objects[(u32)selected_static_object];
    object.name = selected_name;
    object.x = selected_x;
    object.y = selected_y;
    object.z = selected_z;
    current_world->update_static_model_instance((u32)selected_static_chunk,
                                                (u32)selected_static_object);
    set_status("Object updated.");
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_viewport_pick(RML_EVENT_ARGS) {
    bool picked = update_pick_from_event(e);
    if (picked && active_mode == WorldEditorMode::Terrain &&
        !e.GetParameter<bool>("alt_key", false) &&
        brush_type != TerrainBrush::Pick &&
        System::gInput->is_mouse_pressed(MouseEvent::LEFT)) {
        bool changed = false;
        if (current_world != nullptr && !current_world->terrain.is_null()) {
            changed = current_world->terrain->apply_brush(
                last_pick_x, last_pick_y, brush_type, brush_setting);
        }
        if (changed) {
            sync_dirty_maps();
            if (view_model) view_model.DirtyVariable("dirty_maps");
        }
    }

    if (view_model) view_model.DirtyVariable("world_text");
}

void WorldEditor::rml_viewport_scroll(RML_EVENT_ARGS) {
    viewport_scroll_delta += e.GetParameter<f32>("wheel_delta_y", 0.0f);
    e.StopPropagation();
}

void WorldEditor::register_view_model_types(
    Rml::DataModelConstructor &constructor) {
    register_enum<WorldEditorMode>(constructor);
    register_enum<TerrainBrush>(constructor);
    if (auto object = constructor.RegisterStruct<SceneObjectView>()) {
        object.RegisterMember("name", &SceneObjectView::name);
        object.RegisterMember("asset", &SceneObjectView::asset);
        object.RegisterMember("selected", &SceneObjectView::selected);
    }

    if (auto object = constructor.RegisterStruct<TerrainBrushSetting>()) {
        object.RegisterMember("radius", &TerrainBrushSetting::radius);
        object.RegisterMember("strength", &TerrainBrushSetting::strength);
        object.RegisterMember("flatten_height",
                              &TerrainBrushSetting::flatten_height);
        object.RegisterMember("terrain_palette_slot",
                              &TerrainBrushSetting::terrain_palette_slot);
    }
    if (auto object = constructor.RegisterStruct<TerrainPaletteView>()) {
        object.RegisterMember("slot", &TerrainPaletteView::slot);
        object.RegisterMember("selected", &TerrainPaletteView::selected);
    }
    if (auto object = constructor.RegisterStruct<Vec3>()) {
        object.RegisterMember("x", &Vec3::x);
        object.RegisterMember("y", &Vec3::y);
        object.RegisterMember("z", &Vec3::z);
    }
    if (auto object = constructor.RegisterStruct<SkySetting>()) {
        object.RegisterMember("up", &SkySetting::up);
        object.RegisterMember("down", &SkySetting::down);
        object.RegisterMember("left", &SkySetting::left);
        object.RegisterMember("right", &SkySetting::right);
        object.RegisterMember("front", &SkySetting::front);
        object.RegisterMember("back", &SkySetting::back);
    }
    if (auto object = constructor.RegisterStruct<DirectionalLightSetting>()) {
        object.RegisterMember("direction", &DirectionalLightSetting::direction);
        object.RegisterMember("diffuse", &DirectionalLightSetting::diffuse);
        object.RegisterMember("specular", &DirectionalLightSetting::specular);
    }
    if (auto object = constructor.RegisterStruct<WorldSetting>()) {
        object.RegisterMember("name", &WorldSetting::name);
        object.RegisterMember("sky", &WorldSetting::sky);
        object.RegisterMember("directional_light", &WorldSetting::dir_light);
    }
    constructor.RegisterArray<std::vector<SceneObjectView>>();
    constructor.RegisterArray<std::vector<TerrainPaletteView>>();
}

void WorldEditor::bind_view_model_values(
    Rml::DataModelConstructor &constructor) {
    constructor.Bind("world", &world_setting);
    constructor.Bind("editor_mode", &active_mode);
    constructor.Bind("status", &status_text);
    constructor.Bind("dirty_maps", &dirty_maps_text);
    constructor.Bind("viewport_message", &viewport_message);
    constructor.Bind("world_text", &world_text);
    constructor.Bind("brush_setting", &brush_setting);
    constructor.Bind("scene_objects", &scene_objects);
    constructor.Bind("terrain_palette", &terrain_palette);
    constructor.Bind("selected_terrain_diffuse", &selected_terrain_diffuse);
    constructor.Bind("selected_terrain_normal", &selected_terrain_normal);
    constructor.Bind("selected_name", &selected_name);
    constructor.Bind("selected_x", &selected_x);
    constructor.Bind("selected_y", &selected_y);
    constructor.Bind("selected_z", &selected_z);
    constructor.Bind("selected_asset", &selected_asset);
    constructor.Bind("terrain_tool", &brush_type);
    constructor.Bind("has_world", &has_world);
    constructor.Bind("has_status", &has_status);
    constructor.Bind("has_selected_object", &has_selected_object);
    constructor.Bind("show_scene_empty", &show_scene_empty);
    constructor.Bind("show_viewport_empty", &show_viewport_empty);
    constructor.Bind("show_clear_tiles", &show_clear_tiles);
}

void WorldEditor::bind_view_model_events(
    Rml::DataModelConstructor &constructor) {
    constructor.BindEventCallback("set_mode", &WorldEditor::rml_set_mode, this);
    constructor.BindEventCallback("set_tool", &WorldEditor::rml_set_tool, this);
    constructor.BindEventCallback("save_world", &WorldEditor::rml_save_world,
                                  this);
    constructor.BindEventCallback("add_terrain_tile",
                                  &WorldEditor::rml_add_tile, this);
    constructor.BindEventCallback("request_clear_tiles",
                                  &WorldEditor::rml_request_clear_tiles, this);
    constructor.BindEventCallback("cancel_clear_tiles",
                                  &WorldEditor::rml_cancel_clear_tiles, this);
    constructor.BindEventCallback("confirm_clear_tiles",
                                  &WorldEditor::rml_confirm_clear_tiles, this);
    constructor.BindEventCallback(
        "commit_world_settings", &WorldEditor::rml_commit_world_settings, this);
    constructor.BindEventCallback("set_skybox_face",
                                  &WorldEditor::rml_set_skybox_face, this);
    constructor.BindEventCallback("select_terrain_layer",
                                  &WorldEditor::rml_select_terrain_layer, this);
    constructor.BindEventCallback("set_terrain_texture",
                                  &WorldEditor::rml_set_terrain_texture, this);
    constructor.BindEventCallback("select_scene_object",
                                  &WorldEditor::rml_select_scene_object, this);
    constructor.BindEventCallback("commit_selected_object",
                                  &WorldEditor::rml_commit_selected_object,
                                  this);
    constructor.BindEventCallback("viewport_pick",
                                  &WorldEditor::rml_viewport_pick, this);
    constructor.BindEventCallback("viewport_scroll",
                                  &WorldEditor::rml_viewport_scroll, this);
}

void WorldEditor::bind_model(Rml::Context *context) {
    Rml::DataModelConstructor constructor =
        context->CreateDataModel("world_editor");
    if (!constructor) return;

    register_view_model_types(constructor);
    bind_view_model_values(constructor);
    bind_view_model_events(constructor);

    view_model = constructor.GetModelHandle();
    sync_view_model();
    dirty_view_model();
}

}  // namespace Seed
