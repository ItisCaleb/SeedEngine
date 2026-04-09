#include "terrain_editor.h"
#include <imgui.h>
#include "core/concurrency/thread_pool.h"
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/ref.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/input.h"
#include "core/resource/mappable_texture.h"
#include "core/resource/resource_loader.h"
#include "core/resource/texture.h"
#include "core/types.h"
#include "editor/world/editor_terrain.h"
#include "core/math/utils.h"
#include "editor/editor.h"
#include "core/io/file.h"
#include "core/io/dir.h"
#include "core/serialize/json_impl.h"
#include <nfd.h>
#include <string>

namespace Seed {
// ── helpers ─────────────────────────────────────────────
static void te_section_header(const char *label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.8f, 1.f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

// Draw a coloured square swatch
static void te_color_swatch(ImVec4 col, ImVec2 size = ImVec2(20, 20)) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        p, ImVec2(p.x + size.x, p.y + size.y),
        ImGui::ColorConvertFloat4ToU32(col), 3.f);
    ImGui::Dummy(size);
}

// Icon-button that highlights when active
static bool te_tool_button(const char *icon, const char *label, bool is_active,
                           ImVec2 size = ImVec2(48, 48)) {
    if (is_active) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.15f, 0.35f, 0.60f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.20f, 0.42f, 0.72f, 1.f));
    }
    bool clicked = false;
    ImGui::BeginGroup();
    // Push an id so same label on different tools is fine
    ImGui::PushID(label);
    clicked = ImGui::Button(icon, ImVec2(size.x, size.y - 14));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         (size.x - ImGui::CalcTextSize(label).x) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.65f, 1.f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::PopID();
    ImGui::EndGroup();
    if (is_active) ImGui::PopStyleColor(2);
    return clicked;
}
// ── end helpers ─────────────────────────────────────────

void TerrainEditor::init_default_splat_layers() {
    splat_layers = {
        {"Dirt", {}, 0, 1.0f, ImVec4(0.35f, 0.28f, 0.20f, 1.f)},
        {"Grass", {}, 1, 0.72f, ImVec4(0.24f, 0.42f, 0.18f, 1.f)},
        {"Rock", {}, 2, 0.41f, ImVec4(0.47f, 0.44f, 0.40f, 1.f)},
        {"Snow", {}, 3, 0.08f, ImVec4(0.91f, 0.88f, 0.82f, 1.f)},
    };
}

// ── main entry ──────────────────────────────────────────
void TerrainEditor::update() {
    // Available region given to us by the tab
    ImVec2 avail = ImGui::GetContentRegionAvail();

    const float LEFT_W = 210.f;
    const float RIGHT_W = 230.f;
    const float VP_W = avail.x - LEFT_W - RIGHT_W - 8.f;  // 4px gap each side
    const float PANEL_H = avail.y;

    // ── LEFT PANEL ──────────────────────────────────────
    ImGui::BeginChild("##te_left", ImVec2(LEFT_W, PANEL_H), false,
                      ImGuiWindowFlags_NoScrollbar);
    draw_left_panel();
    ImGui::EndChild();

    ImGui::SameLine(0, 4);

    // ── VIEWPORT ────────────────────────────────────────
    ImGui::BeginChild(
        "##te_viewport", ImVec2(VP_W, PANEL_H), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (VP_W > 0 && PANEL_H > 0) {
        draw_viewport(VP_W, PANEL_H);
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 4);

    // ── RIGHT PANEL ─────────────────────────────────────
    ImGui::BeginChild("##te_right", ImVec2(RIGHT_W, PANEL_H), false);
    draw_right_panel();
    ImGui::EndChild();
}

// ─────────────────────────────────────────────────────────
//  LEFT PANEL —  brush settings
// ─────────────────────────────────────────────────────────
void TerrainEditor::draw_left_panel() {
    // ── Brush Settings ───────────────────────────────────
    te_section_header("BRUSH");

    ImGui::PushItemWidth(-1);

    ImGui::Text("Radius");
    ImGui::SliderFloat("##br", &brush_radius, 1.f, 50.f, "%.0f");

    ImGui::Text("Strength");
    ImGui::SliderFloat("##bs", &brush_strength, 0.01f, 1.f, "%.2f");

    ImGui::Text("Falloff");
    ImGui::SliderFloat("##bf", &brush_falloff, 0.f, 1.f, "%.2f");

    ImGui::Text("Opacity");
    ImGui::SliderFloat("##bo", &brush_opacity, 0.f, 1.f, "%.2f");

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Text("Shape");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
    static int brush_shape = 1;  // 0=circle 1=gauss 2=square
    if (ImGui::RadioButton("Circle", brush_shape == 0)) brush_shape = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Gauss", brush_shape == 1)) brush_shape = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("Square", brush_shape == 2)) brush_shape = 2;
    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Spacing();

    // ── Height Range ─────────────────────────────────────
    te_section_header("HEIGHT RANGE");

    ImGui::PushItemWidth(80);
    static float h_min = 0.f, h_max = 255.f, h_target = 128.f;
    ImGui::Text("Min");
    ImGui::SameLine();
    ImGui::InputFloat("##hmin", &h_min, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::Text("Max");
    ImGui::SameLine();
    ImGui::InputFloat("##hmax", &h_max, 0, 0, "%.0f");
    ImGui::PopItemWidth();

    ImGui::Text("Target");
    ImGui::PushItemWidth(-1);
    ImGui::SliderFloat("##ht", &h_target, h_min, h_max, "%.0f");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();

    // ── Bottom buttons ────────────────────────────────────
    // Push to the bottom of the child window
    float btn_area_h = 4 * (ImGui::GetFrameHeightWithSpacing()) + 12.f;
    float space = ImGui::GetContentRegionAvail().y - btn_area_h;
    if (space > 0) ImGui::Dummy(ImVec2(0, space));

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.60f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.20f, 0.42f, 0.72f, 1.f));
    if (ImGui::Button("+ New Terrain", ImVec2(-1, 0)))
        show_new_terrain_modal = true;
    if (ImGui::Button("Load Terrain", ImVec2(-1, 0))) {
        nfdu8char_t *path;
        nfdopendialogu8args_t args = {0};
        nfdresult_t r = NFD_OpenDialogU8_With(&path, &args);
        if (r == NFD_OKAY) {
            load_terrain(path);
        }
    }
    ImGui::PopStyleColor(2);
    if (this->current_terrain.is_valid()) {
        if (ImGui::Button("Import Heightmap", ImVec2(-1, 0))) {
            // TODO: open file dialog
            // e.g. ImGuiFileDialog::Instance()->OpenDialog(...)
        }

        if (ImGui::Button("Save terrain", ImVec2(-1, 0))) {
            this->current_terrain->dump(gEditor->project()->get_asset_dir());
        }
    }
}

void TerrainEditor::load_terrain(const Path &path) {
    ResourceLoader *loader = ResourceLoader::get_instance();
    Ref<File> file = File::open(path, "rb");
    Ref<Dir> dir = Dir::open(file->get_directory());
    auto terrain_info = file->read_json();
    std::string name = terrain_info["name"];
    u32 width = terrain_info["width"];
    u32 height = terrain_info["height"];
    Ref<MappableTexture> height_map, splat_map, light_map;
    auto jheight_map = terrain_info["height_map"];
    height_map = loader->load<MappableTexture>(jheight_map);

    auto jsplat_map = terrain_info["splat_map"];
    splat_map = loader->load<MappableTexture>(jsplat_map);
    if (terrain_info.contains("light_map")) {
        auto jlight_map = terrain_info["light_map"];
        light_map = loader->load<MappableTexture>(jlight_map);
    }
    this->current_terrain.create(width, height, height_map, splat_map,
                                 light_map);
    // if (terrain_info.contains("tex1")) {
    //     auto jtex1 = terrain_info["tex1"];
    //     auto texture = loader->load<Texture>(dir->concat(jtex1));
    //     this->current_terrain->get_material()->set_texture("tex1", texture);
    //     texture->update_sampler(SamplerProperty{.wrap_u =
    //     SamplerWrap::REPEAT,
    //                                             .wrap_v =
    //                                             SamplerWrap::REPEAT});
    // }

    // if (terrain_info.contains("tex1_normal")) {
    //     auto jtex1 = terrain_info["tex1_normal"];
    //     auto texture = loader->load<Texture>(dir->concat(jtex1));
    //     this->current_terrain->get_material()->set_texture("tex1_normal",
    //                                                        texture);
    //     texture->update_sampler(SamplerProperty{.wrap_u =
    //     SamplerWrap::REPEAT,
    //                                             .wrap_v =
    //                                             SamplerWrap::REPEAT});
    // }
    gEditor->set_last_open_world(path);
    this->current_terrain->name = name;
}

// ─────────────────────────────────────────────────────────
//  VIEWPORT
// ─────────────────────────────────────────────────────────
void TerrainEditor::draw_viewport_toolbar() {
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 s = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y),
                                              IM_COL32(20, 22, 26, 180), 4.f);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    ImGui::SetCursorPosY(4);

    struct ToolDef {
            const char *label;
            TerrainTool tool;
    };
    static const ToolDef tools[] = {
        {"Raise", TerrainTool::Raise},   {"Lower", TerrainTool::Lower},
        {"Smooth", TerrainTool::Smooth}, {"Flatten", TerrainTool::Flatten},
        {"Noise", TerrainTool::Noise},   {"Erode", TerrainTool::Erode},
        {"Paint", TerrainTool::Paint},   {"Pick", TerrainTool::Pick},
    };

    for (auto &t : tools) {
        bool active = (active_tool == t.tool);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.15f, 0.35f, 0.60f, 1.f));
        if (ImGui::SmallButton(t.label)) active_tool = t.tool;
        if (active) ImGui::PopStyleColor();
        ImGui::SameLine(0, 2);
    }

    // ── 分隔線 ───────────────────────────────────────────
    ImGui::SameLine(0, 8);
    ImVec2 _p = ImGui::GetCursorScreenPos();
    float h = ImGui::GetFrameHeight();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(_p.x, _p.y),
                                        ImVec2(_p.x, _p.y + h),
                                        IM_COL32(80, 80, 80, 255), 1.f);
    ImGui::Dummy(ImVec2(1.f, h));
    ImGui::SameLine(0, 8);

    // ── Brush shape ──────────────────────────────────────
    // const char *shapes[] = { "O", "G", "[]" }; // Circle/Gauss/Square 縮寫
    // for (int i = 0; i < 3; i++) {
    //     bool active = (brush_shape == i);
    //     if (active) ImGui::PushStyleColor(ImGuiCol_Button,
    //                                       ImVec4(0.15f, 0.35f, 0.60f, 1.f));
    //     if (ImGui::SmallButton(shapes[i])) brush_shape = i;
    //     if (active) ImGui::PopStyleColor();
    //     if (i < 2) ImGui::SameLine(0, 2);
    // }

    // ImGui::SameLine(0, 8);
    // ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    // ImGui::SameLine(0, 8);

    // ── 工具專屬參數 ─────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // radius + strength 幾乎所有工具都要
    auto slider_labeled = [](const char *label, const char *id, float *v,
                             float mn, float mx, float w) {
        ImGui::Text("%s", label);
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(w);
        ImGui::SliderFloat(id, v, mn, mx, "%.2f");
    };

    // switch (active_tool) {
    //     case TerrainTool::Raise:
    //     case TerrainTool::Lower:
    //     case TerrainTool::Noise:
    //     case TerrainTool::Erode:
    //         slider_labeled("r",   "##vp_br",
    //         &brush_radius,   1.f,  50.f, 70.f); ImGui::SameLine(0, 8);
    //         slider_labeled("str", "##vp_bs", &brush_strength,
    //         0.01f, 1.f, 70.f); break;

    //     case TerrainTool::Smooth:
    //         slider_labeled("str", "##vp_bs", &brush_strength,
    //         0.01f, 1.f, 70.f); ImGui::SameLine(0, 8); ImGui::Text("iter");
    //         ImGui::SameLine(0, 4);
    //         ImGui::SetNextItemWidth(40);
    //         ImGui::InputInt("##si", &smooth_iterations, 0);
    //         smooth_iterations = std::clamp(smooth_iterations, 1, 16);
    //         break;

    //     case TerrainTool::Flatten:
    //         slider_labeled("r",      "##vp_br",
    //         &brush_radius,  1.f,   50.f, 70.f); ImGui::SameLine(0, 8);
    //         slider_labeled("target", "##vp_ht", &h_target, h_min,
    //         h_max, 90.f); break;

    //     case TerrainTool::Paint:
    //         if (active_layer < (int)splat_layers.size()) {
    //             auto &layer = splat_layers[active_layer];
    //             // 色塊
    //             ImVec2 sw = ImGui::GetCursorScreenPos();
    //             ImGui::GetWindowDrawList()->AddRectFilled(
    //                 sw, ImVec2(sw.x + 14, sw.y + 14),
    //                 ImGui::ColorConvertFloat4ToU32(layer.preview_color), 2.f);
    //             ImGui::Dummy(ImVec2(14, 14));
    //             ImGui::SameLine(0, 4);
    //             ImGui::TextUnformatted(layer.name.c_str());
    //             ImGui::SameLine(0, 8);
    //             // 快速切換 layer
    //             if (ImGui::ArrowButton("##prev_layer", ImGuiDir_Left))
    //                 active_layer = std::max(0, active_layer - 1);
    //             ImGui::SameLine(0, 2);
    //             if (ImGui::ArrowButton("##next_layer", ImGuiDir_Right))
    //                 active_layer = std::min((int)splat_layers.size() - 1,
    //                                         active_layer + 1);
    //             ImGui::SameLine(0, 8);
    //             slider_labeled("flow", "##vp_pf", &paint_flow,
    //             0.f, 1.f, 70.f);
    //         }
    //         break;

    //     case TerrainTool::Pick:
    //         ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f,
    //         0.9f, 1.f)); ImGui::Text("Coord: %d, %d   H: %d",
    //                     last_click_x, last_click_y, last_pick_height);
    //         ImGui::PopStyleColor();
    //         break;

    //     default:
    //         break;
    // }

    ImGui::PopStyleVar(3);
}

void TerrainEditor::draw_viewport(float vp_w, float vp_h) {
    u32 need_w = (u32)vp_w;
    u32 need_h = (u32)(vp_h - ImGui::GetFrameHeightWithSpacing() - 22.f);

    if (need_w != screen_width || need_h != screen_height) {
        screen_width = need_w;
        screen_height = need_h;

        screen_texture.create(TextureType::TEXTURE_2D, screen_width,
                              screen_height, PixelFormat::RGBA, nullptr);
        screen_depth.create(TextureType::TEXTURE_2D, screen_width,
                            screen_height, PixelFormat::D32, nullptr);
        picking_texture.create(TextureType::TEXTURE_2D, screen_width,
                               screen_height, PixelFormat::RGBA16I, nullptr);
        renderer->rebind_textures(screen_texture, screen_depth,
                                  picking_texture);
    }
    // Status tags at top
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.5f, 1.f));
    if (current_terrain.is_valid())
        ImGui::Text("%dx%d", current_terrain->get_width(),
                    current_terrain->get_height());
    else
        ImGui::TextUnformatted("No terrain loaded");
    ImGui::PopStyleColor();

    ImGui::SameLine(vp_w - 180.f);

    // View mode toggle
    static int view_mode = 1;  // 0=heightmap 1=3D 2=splat
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
    if (ImGui::RadioButton("HM", view_mode == 0)) view_mode = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("3D", view_mode == 1)) view_mode = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("Splat", view_mode == 2)) view_mode = 2;
    ImGui::PopStyleVar();

    // Actual render image
    ImVec2 img_origin = ImGui::GetCursorScreenPos();
    float img_h = vp_h - ImGui::GetCursorPosY() - 30.f;  // leave status bar

    if (current_terrain.is_valid()) {
        edit_terrain_imgui(img_origin, vp_w, img_h);
    } else {
        // Empty state
        ImVec2 center =
            ImVec2(img_origin.x + vp_w * 0.5f, img_origin.y + img_h * 0.5f);
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(img_origin,
                          ImVec2(img_origin.x + vp_w, img_origin.y + img_h),
                          IM_COL32(20, 22, 26, 255));
        const char *msg = "No terrain  —  press  '+ New Terrain'";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
                    IM_COL32(80, 90, 100, 255), msg);
        ImGui::Dummy(ImVec2(vp_w, img_h));
    }

    // Status bar
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.55f, 0.6f, 1.f));
    if (current_terrain.is_valid()) {
        ImGui::Text("Tool: %s    Brush r=%.0f  str=%.2f    Coord: %d, %d",
                    tool_name(active_tool), brush_radius, brush_strength,
                    last_click_x, last_click_y);
    } else {
        ImGui::TextUnformatted("Ready");
    }
    ImGui::PopStyleColor();
}

void TerrainEditor::init() {
    screen_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                          PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                        PixelFormat::D32, nullptr);
    picking_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                           PixelFormat::RGBA16I, nullptr);

    renderer = new TerrainEditorRenderer(screen_texture, screen_depth,
                                         picking_texture);
    RenderEngine::get_instance()->register_renderer(1, renderer);
}

// Replaces the old edit_terrain() — called only when terrain is valid
void TerrainEditor::edit_terrain_imgui(ImVec2 origin, float w, float h) {
    // Render scene into screen_texture as before, then display
    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(), ImVec2(w, h),
                 ImVec2(0, 1), ImVec2(1, 0));

    float toolbar_h = 32.f;
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 4, origin.y + 4));
    ImGui::BeginChild(
        "##vp_toolbar", ImVec2(w - 8, toolbar_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    draw_viewport_toolbar();
    ImGui::EndChild();

    ImVec2 mouse = ImGui::GetMousePos();
    bool in_toolbar = mouse.y < origin.y + toolbar_h + 4.f;
    if (!ImGui::IsWindowHovered() || in_toolbar) return;
    Input *input = Input::get_instance();

    Viewport vp = Viewport{(u32)w, (u32)h};
    auto vp_coord =
        vp.to_viewport_coord(Vec2{mouse.x - origin.x, mouse.y - origin.y});

    u32 x = (u32)vp_coord.x;
    u32 y = (u32)vp_coord.y;

    auto terrain_coord = (i16 *)picking_texture->pixel_repeat(x, (u32)h - y);

    i16 tx = terrain_coord[0] + current_terrain->get_width() / 2;
    i16 ty = terrain_coord[1] + current_terrain->get_height() / 2;
    last_click_x = tx;
    last_click_y = ty;

    if (input->is_mouse_pressed(MouseEvent::LEFT)) {
        apply_brush(tx, ty);
    }

    // Brush circle overlay
    ImDrawList *dl = ImGui::GetWindowDrawList();
    float screen_radius = brush_radius * (w / current_terrain->get_width());
    dl->AddCircle(mouse, screen_radius, IM_COL32(120, 180, 255, 200), 32, 1.f);
    dl->AddCircle(mouse, screen_radius * 0.3f, IM_COL32(120, 180, 255, 80), 16,
                  1.f);
}

void TerrainEditor::apply_brush(i16 cx, i16 cy) {
    Ref<MappableTexture> height_map = current_terrain->get_heightmap();
    // Ref<MappableTexture> splat_map = current_terrain->get_splatmap();
    i32 r = (u32)brush_radius;

    for (i32 i = -r; i <= r; i++) {
        for (i32 j = -r; j <= r; j++) {
            float dist = sqrtf((float)(i * i + j * j));
            if (dist > r) continue;

            // Gaussian falloff
            float falloff =
                expf(-dist * dist / (2.f * r * r * brush_falloff + 0.001f));
            float delta = brush_strength * falloff * 0.5f;

            u32 px = (u32)(cx + i);
            u32 py = (u32)(cy + j);
            if (px >= height_map->get_width() || py >= height_map->get_height())
                continue;

            u8 *c = height_map->pixel(px, py);
            switch (active_tool) {
                case TerrainTool::Raise:
                    c[1] = (u8)std::min(255, (i32)c[1] + (i32)(delta * 255));
                    break;
                case TerrainTool::Lower:
                    c[1] = (u8)std::max(0, (i32)c[1] - (i32)(delta * 255));
                    break;
                case TerrainTool::Smooth: {
                    // Average with neighbours (simple box)
                    i32 sum = 0, cnt = 0;
                    for (i32 di = -2; di <= 2; di++)
                        for (i32 dj = -2; dj <= 2; dj++) {
                            i32 nx = px + di, ny = py + dj;
                            if (nx < height_map->get_width() &&
                                ny < height_map->get_height()) {
                                sum += height_map->pixel(nx, ny)[1];
                                cnt++;
                            }
                        }
                    i32 avg = sum / cnt;
                    c[1] = (u8)(c[1] + (avg - c[1]) * falloff * brush_strength);
                    break;
                }
                case TerrainTool::Flatten:
                    // Blend towards target height
                    c[1] = (u8)(c[1] +
                                (128 - c[1]) * falloff * brush_strength * 0.1f);
                    break;
                // case TerrainTool::Paint:
                //     c = splat_map->pixel(px, py);
                //     c[0] = 255;
                //     break;
                default:
                    break;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────
//  RIGHT PANEL — texture splatting
// ─────────────────────────────────────────────────────────
void TerrainEditor::draw_right_panel() {
    te_section_header("TEXTURE SPLATTING");

    // ── Layer list ───────────────────────────────────────
    if (ImGui::Button("+ Add Layer", ImVec2(-1, 0))) {
        splat_layers.push_back({"New Layer",
                                {},
                                (int)splat_layers.size() % 4,
                                0.5f,
                                ImVec4(0.5f, 0.5f, 0.5f, 1.f)});
    }
    ImGui::Spacing();

    static const char *channel_names[] = {"R", "G", "B", "A"};

    for (int i = 0; i < (int)splat_layers.size(); i++) {
        SplatLayer &layer = splat_layers[i];
        bool is_active = (i == active_layer);

        ImGui::PushID(i);

        // Highlight active layer
        if (is_active) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg,
                                  ImVec4(0.08f, 0.18f, 0.32f, 1.f));
        }

        ImGui::BeginChild("##layer", ImVec2(-1, 54), true,
                          ImGuiWindowFlags_NoScrollbar);

        // Swatch
        te_color_swatch(layer.preview_color);
        ImGui::SameLine(0, 8);

        // Name + channel + weight bar
        ImGui::BeginGroup();
        if (is_active) {
            char buf[64];
            strncpy(buf, layer.name.c_str(), 63);
            ImGui::PushItemWidth(100);
            if (ImGui::InputText("##lname", buf, 64)) layer.name = buf;
            ImGui::PopItemWidth();
        } else {
            ImGui::TextUnformatted(layer.name.c_str());
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.55f, 0.6f, 1.f));
        ImGui::Text("Ch %s  |  w=%.2f", channel_names[layer.channel],
                    layer.weight);
        ImGui::PopStyleColor();

        // Weight bar
        ImVec2 bar_pos = ImGui::GetCursorScreenPos();
        float bar_w = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddRectFilled(
            bar_pos, ImVec2(bar_pos.x + bar_w, bar_pos.y + 3),
            IM_COL32(40, 44, 50, 255), 2);
        ImGui::GetWindowDrawList()->AddRectFilled(
            bar_pos, ImVec2(bar_pos.x + bar_w * layer.weight, bar_pos.y + 3),
            ImGui::ColorConvertFloat4ToU32(
                ImVec4(layer.preview_color.x * 0.8f,
                       layer.preview_color.y * 0.8f + 0.2f,
                       layer.preview_color.z * 0.7f + 0.1f, 1.f)),
            2);
        ImGui::Dummy(ImVec2(bar_w, 3));
        ImGui::EndGroup();

        ImGui::EndChild();
        if (is_active) ImGui::PopStyleColor();

        if (ImGui::IsItemClicked()) active_layer = i;

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // ── Paint settings ───────────────────────────────────
    if (ImGui::CollapsingHeader("Paint Settings",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushItemWidth(-1);
        ImGui::Text("Flow");
        ImGui::SliderFloat("##pf", &paint_flow, 0.f, 1.f, "%.2f");
        ImGui::Text("Hardness");
        ImGui::SliderFloat("##ph", &paint_hardness, 0.f, 1.f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::Checkbox("Slope-based auto blend", &slope_blend);
        ImGui::Checkbox("Height-based auto blend", &height_blend);
        if (slope_blend || height_blend) {
            ImGui::Text("Threshold");
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##pt", &blend_threshold, 0.f, 1.f, "%.2f");
            ImGui::PopItemWidth();
        }
    }

    ImGui::Spacing();

    // ── Texture tiling ────────────────────────────────────
    if (ImGui::CollapsingHeader("Texture Tiling")) {
        ImGui::PushItemWidth(80);
        ImGui::Text("Tile U");
        ImGui::SameLine();
        ImGui::InputFloat("##tu", &tile_u, 0.5f, 1.f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("V");
        ImGui::SameLine();
        ImGui::InputFloat("##tv", &tile_v, 0.5f, 1.f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::Checkbox("Normal map enabled", &normal_map);
    }

    // ── Bottom buttons ────────────────────────────────────
    float btn_h = 2 * (ImGui::GetFrameHeightWithSpacing()) + 12.f;
    float space = ImGui::GetContentRegionAvail().y - btn_h;
    if (space > 0) ImGui::Dummy(ImVec2(0, space));

    ImGui::Separator();
    ImGui::Spacing();

    if (current_terrain.is_valid()) {
        if (ImGui::Button("Bake Splatmap", ImVec2(-1, 0))) {
            // TODO: bake splat_layers weights into splatmap texture
        }
        ImGui::BeginDisabled(lightmap_baking);
        if (ImGui::Button("Bake Lightmap", ImVec2(-1, 0))) {
            lightmap_baking = true;
            ThreadPool::get_instance()->add_work([&](void *) {
                this->current_terrain->gen_lightmap();
                lightmap_baking = false;
            });
        }
        ImGui::EndDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.40f, 0.12f, 0.12f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.55f, 0.15f, 0.15f, 1.f));
        if (ImGui::Button("Clear Layer", ImVec2(-1, 0))) {
            // TODO: clear active layer's splatmap channel
        }
        ImGui::PopStyleColor(2);
    }
}



// ── utility ─────────────────────────────────────────────
const char *TerrainEditor::tool_name(TerrainTool t) {
    switch (t) {
        case TerrainTool::Raise:
            return "Raise";
        case TerrainTool::Lower:
            return "Lower";
        case TerrainTool::Smooth:
            return "Smooth";
        case TerrainTool::Flatten:
            return "Flatten";
        case TerrainTool::Noise:
            return "Noise";
        case TerrainTool::Erode:
            return "Erode";
        case TerrainTool::Paint:
            return "Paint";
        case TerrainTool::Pick:
            return "Pick";
        default:
            return "Unknown";
    }
}
}  // namespace Seed
