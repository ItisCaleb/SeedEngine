#include "world_editor.h"
#include "core/input.h"
#include "editor/world/editor_world.h"
#include <fmt/format.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/misc/enums.h"
#include "core/project.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include "editor/world/world_renderer.h"

namespace Seed {

WorldEditor::~WorldEditor() { delete current_world; }

bool WorldEditor::load_world(const UUID uuid) {
    status_text.clear();

    if (gEditor != nullptr && gEditor->ctx.current_inspect != nullptr) {
        gEditor->set_current_inspect(nullptr);
    }

    delete current_world;
    current_world = nullptr;
    current_entry =
        ResourceLoader::get_instance()->get_entries().get_entry(uuid);
    if (current_entry == nullptr) {
        status_text = "World file is not registered in resource entries.";
        return false;
    }

    current_world = new EditorWorld(current_entry);

    selected_static_chunk = -1;
    selected_static_object = -1;
    last_pick_valid = false;

    set_current_world_inspector();
    if (gEditor != nullptr) {
        gEditor->set_last_open_world(current_entry->uuid);
    }
    status_text = "World loaded.";
    return true;
}

void WorldEditor::init() {
    screen_texture.create(TextureType::TEXTURE_2D, texture_width,
                          texture_height, PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, texture_width, texture_height,
                        PixelFormat::D32, nullptr);
    picking_texture.create(TextureType::TEXTURE_2D, texture_width,
                           texture_height, PixelFormat::RGBA16I, nullptr);

    renderer = new WorldRenderer(screen_texture, screen_depth,
                                 ref_cast<Texture>(picking_texture));
    GuiEngine::get_instance()->add_texture("main_view", screen_texture);
    RenderEngine::get_instance()->register_renderer(1, renderer);
}

void WorldEditor::set_current_world_inspector() {
    if (gEditor == nullptr || current_world == nullptr) return;
    gEditor->set_current_inspect(new EditorWorldInspector(current_world));
}

bool WorldEditor::chunk_exists_at(i32 chunk_x, i32 chunk_y) const {
    return current_world != nullptr &&
           current_world->terrain_chunk_exists_at(chunk_x, chunk_y);
}

std::string WorldEditor::static_model_label(UUID uuid) const {
    ResourceEntry *entry =
        ResourceLoader::get_instance()->get_entries().get_entry(uuid);
    if (entry == nullptr) return "Missing Model";
    KStr name = entry->path.filename();
    return std::string(name.data(), name.length());
}

void WorldEditor::select_static_object(u32 chunk_index, u32 object_index) {
    if (current_world == nullptr) return;
    selected_static_chunk = (i32)chunk_index;
    selected_static_object = (i32)object_index;
    gEditor->set_current_inspect(new EditorStaticObjectInspector(
        current_world, chunk_index, object_index));
}

void WorldEditor::save_current_world() {
    if (current_world == nullptr) return;
    current_world->save_dirty_terrain_maps();
    current_world->save();

    Project *project = SeedEngine::get_instance()->get_project();
    if (project != nullptr) {
        ResourceLoader::get_instance()->get_entries().save(
            project->get_entry_path());
        status_text = "World saved through resource entries.";
    }
}

bool WorldEditor::add_chunk_at(i32 chunk_x, i32 chunk_y) {
    if (current_world == nullptr) return false;
    if (chunk_exists_at(chunk_x, chunk_y)) {
        status_text = "Terrain tile already exists.";
        return false;
    }

    if (!current_world->add_new_chunk(chunk_x, chunk_y)) {
        status_text = "Failed to add terrain tile.";
        return false;
    }

    save_current_world();
    status_text = fmt::format("Added terrain tile ({}, {}).", chunk_x, chunk_y);
    return true;
}

void WorldEditor::add_chunk() {
    if (current_world == nullptr) return;
    std::vector<ChunkSetting> chunks = current_world->get_chunks();
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
    if (current_world == nullptr || gEditor == nullptr) return;

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
        gEditor->remove_asset(uuid);
    }

    current_world->clear_tiles();
    last_pick_valid = false;
    save_current_world();
    status_text = "All terrain tiles cleared.";
}

void WorldEditor::sync_scene_view_model() {
    scene_objects.clear();
    if (current_world == nullptr) {
        has_scene_objects = false;
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
            view.name = object.name.is_empty()
                            ? KString(static_model_label(object.model))
                            : object.name;
            view.asset = static_model_label(object.model);
            view.selected = selected_static_chunk == (i32)chunk_index &&
                            selected_static_object == (i32)object_index;
            view.chunk_index = (i32)chunk_index;
            view.object_index = (i32)object_index;
            scene_objects.push_back(view);
        }
    }

    has_scene_objects = !scene_objects.empty();
    show_scene_empty = scene_objects.empty();
}

void WorldEditor::sync_selected_object_view_model() {
    has_selected_object = false;
    show_inspector_empty = true;
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
    show_inspector_empty = false;
}

void WorldEditor::sync_view_model() {
    world_mode = active_mode == WorldEditorMode::World;
    terrain_mode = active_mode == WorldEditorMode::Terrain;
    mode_text = world_mode ? "World" : "Terrain";
    has_world = current_world != nullptr;
    has_status = !status_text.empty();
    dirty_maps_text =
        current_world != nullptr && current_world->has_dirty_terrain_maps()
            ? "Yes"
            : "No";

    if (current_world == nullptr) {
        viewport_message = "Open a world from Assets.";
        world_text = "-";
        show_viewport_empty = true;
        sync_scene_view_model();
        sync_selected_object_view_model();
        return;
    }

    show_viewport_empty = current_world->get_chunks().empty();
    viewport_message = show_viewport_empty ? "Add a terrain tile." : "";
    if (!last_pick_valid) world_text = "-";

    sync_scene_view_model();
    sync_selected_object_view_model();
}

void WorldEditor::dirty_view_model() {
    if (!view_model) return;
    view_model.DirtyVariable("world_mode");
    view_model.DirtyVariable("terrain_mode");
    view_model.DirtyVariable("mode");
    view_model.DirtyVariable("status");
    view_model.DirtyVariable("dirty_maps");
    view_model.DirtyVariable("viewport_message");
    view_model.DirtyVariable("world_text");
    view_model.DirtyVariable("brush_setting");
    view_model.DirtyVariable("scene_objects");
    view_model.DirtyVariable("selected_name");
    view_model.DirtyVariable("selected_x");
    view_model.DirtyVariable("selected_y");
    view_model.DirtyVariable("selected_z");
    view_model.DirtyVariable("selected_asset");
    view_model.DirtyVariable("terrain_tool");
    view_model.DirtyVariable("has_world");
    view_model.DirtyVariable("has_status");
    view_model.DirtyVariable("has_scene_objects");
    view_model.DirtyVariable("has_selected_object");
    view_model.DirtyVariable("show_scene_empty");
    view_model.DirtyVariable("show_inspector_empty");
    view_model.DirtyVariable("show_viewport_empty");
    view_model.DirtyVariable("show_clear_tiles");
}

bool WorldEditor::viewport_event_to_pixel(Rml::Event &event, i32 &image_x,
                                          i32 &image_y) const {
    Rml::Element *element = event.GetCurrentElement();
    if (element == nullptr) return false;

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
    if (picking_texture.is_null()) return false;
    if (image_x < 0 || image_y < 0 || image_x >= (i32)texture_width ||
        image_y >= (i32)texture_height) {
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
    Rml::String mode = args[0].Get<Rml::String>("");
    if (mode == "world") active_mode = WorldEditorMode::World;
    if (mode == "terrain") active_mode = WorldEditorMode::Terrain;
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_set_tool(RML_EVENT_ARGS) {
    Rml::String str = args[0].Get<Rml::String>("");
    brush_type = string_to_enum<TerrainBrush>(str);

    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_save_world(RML_EVENT_ARGS) {
    save_current_world();
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_inspect_world(RML_EVENT_ARGS) {
    set_current_world_inspector();
}

void WorldEditor::rml_add_tile(RML_EVENT_ARGS) {
    add_chunk();
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_request_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = true;
    dirty_view_model();
}

void WorldEditor::rml_cancel_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = false;
    dirty_view_model();
}

void WorldEditor::rml_confirm_clear_tiles(RML_EVENT_ARGS) {
    show_clear_tiles = false;
    clear_tiles();
    sync_view_model();
    dirty_view_model();
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
    status_text = "Object updated.";
    sync_view_model();
    dirty_view_model();
}

void WorldEditor::rml_viewport_pick(RML_EVENT_ARGS) {
    bool picked = update_pick_from_event(e);
    if (picked && terrain_mode && brush_type != TerrainBrush::Pick &&
        Input::get_instance()->is_mouse_pressed(MouseEvent::LEFT)) {
        if (current_world != nullptr && !current_world->terrain.is_null()) {
            current_world->terrain->apply_brush(last_pick_x, last_pick_y,
                                                brush_type, brush_setting);
        }
        sync_view_model();
        dirty_view_model();
        return;
    }

    if (view_model) view_model.DirtyVariable("world_text");
}

void WorldEditor::bind_model(Rml::Context *context) {
    Rml::DataModelConstructor constructor =
        context->CreateDataModel("world_editor");
    if (!constructor) return;
    constructor.RegisterScalar<TerrainBrush>(
        [](const TerrainBrush &tool, Rml::Variant &variant) {
            variant = Rml::String(enum_to_string(tool));
        },
        [](TerrainBrush &tool, const Rml::Variant &variant) {
            Rml::String str = variant.Get<Rml::String>();
            tool = string_to_enum<TerrainBrush>(str);
        });
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
    }
    constructor.RegisterArray<std::vector<SceneObjectView>>();
    constructor.Bind("world_mode", &world_mode);
    constructor.Bind("terrain_mode", &terrain_mode);
    constructor.Bind("mode", &mode_text);
    constructor.Bind("status", &status_text);
    constructor.Bind("dirty_maps", &dirty_maps_text);
    constructor.Bind("viewport_message", &viewport_message);
    constructor.Bind("world_text", &world_text);
    constructor.Bind("brush_setting", &brush_setting);
    constructor.Bind("scene_objects", &scene_objects);
    constructor.Bind("selected_name", &selected_name);
    constructor.Bind("selected_x", &selected_x);
    constructor.Bind("selected_y", &selected_y);
    constructor.Bind("selected_z", &selected_z);
    constructor.Bind("selected_asset", &selected_asset);
    constructor.Bind("terrain_tool", &brush_type);
    constructor.Bind("has_world", &has_world);
    constructor.Bind("has_status", &has_status);
    constructor.Bind("has_scene_objects", &has_scene_objects);
    constructor.Bind("has_selected_object", &has_selected_object);
    constructor.Bind("show_scene_empty", &show_scene_empty);
    constructor.Bind("show_inspector_empty", &show_inspector_empty);
    constructor.Bind("show_viewport_empty", &show_viewport_empty);
    constructor.Bind("show_clear_tiles", &show_clear_tiles);
    constructor.BindEventCallback("set_mode", &WorldEditor::rml_set_mode, this);
    constructor.BindEventCallback("set_tool", &WorldEditor::rml_set_tool, this);
    constructor.BindEventCallback("save_world", &WorldEditor::rml_save_world,
                                  this);
    constructor.BindEventCallback("inspect_world",
                                  &WorldEditor::rml_inspect_world, this);
    constructor.BindEventCallback("add_terrain_tile",
                                  &WorldEditor::rml_add_tile, this);
    constructor.BindEventCallback("request_clear_tiles",
                                  &WorldEditor::rml_request_clear_tiles, this);
    constructor.BindEventCallback("cancel_clear_tiles",
                                  &WorldEditor::rml_cancel_clear_tiles, this);
    constructor.BindEventCallback("confirm_clear_tiles",
                                  &WorldEditor::rml_confirm_clear_tiles, this);
    constructor.BindEventCallback("select_scene_object",
                                  &WorldEditor::rml_select_scene_object, this);
    constructor.BindEventCallback("commit_selected_object",
                                  &WorldEditor::rml_commit_selected_object,
                                  this);
    constructor.BindEventCallback("viewport_pick",
                                  &WorldEditor::rml_viewport_pick, this);

    view_model = constructor.GetModelHandle();
    sync_view_model();
    dirty_view_model();
}

}  // namespace Seed
