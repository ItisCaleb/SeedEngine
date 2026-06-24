#ifndef _SEED_ASSET_BROWSER_H_
#define _SEED_ASSET_BROWSER_H_

#include <imgui.h>
#include <map>
#include <mutex>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/resource/image.h"
#include "core/types.h"
#include "editor/asset/asset.h"
#include "editor/gui/inspectable.h"

namespace Seed {

class AssetBrowser {
    public:
        void init(KStr project_root);
        void update();
        Ref<Dir> get_current_dir() { return current_dir; }

    private:
        Ref<Dir> root_dir;
        Ref<Dir> current_dir;
        std::vector<AssetEntry> entries;
        std::map<Path, std::vector<AssetEntry>> folder_entry_cache;
        i32 selected_idx = -1;
        f32 folder_panel_width = 220.f;
        f32 icon_size = 72.f;
        char search_buf[128] = {};
        bool needs_refresh = true;
        i32 renaming_idx = -1;
        char rename_buf[256] = {};
        std::vector<Path> breadcrumbs;

        struct ThumbnailResult {
                Path path;
                Ref<Image> image;
                u32 source_width = 0;
                u32 source_height = 0;
                bool failed = false;
        };

        struct GridLayout {
                f32 cell_width = 156.f;
                f32 cell_height = 0.f;
                f32 padding = 8.f;
                i32 columns = 1;
                ImVec2 start;
        };

        struct AssetCellRects {
                ImVec2 min;
                ImVec2 max;
                ImVec2 icon_min;
                ImVec2 icon_max;
                ImVec2 label_min;
                ImVec2 label_max;
        };

        std::mutex thumbnail_results_mutex;
        std::vector<ThumbnailResult> thumbnail_results;
        void refresh();
        void invalidate_current_folder_cache();
        void process_thumbnail_results();
        void queue_thumbnail_result(ThumbnailResult result);
        bool apply_thumbnail_result(std::vector<AssetEntry> &target_entries,
                                    const ThumbnailResult &result,
                                    Ref<Texture> texture);
        void navigate_to(KStr dir);
        void draw_breadcrumb();
        void draw_toolbar();
        void draw_folder_panel();
        void draw_folder_tree(const Path &dir);
        GridLayout make_grid_layout() const;
        AssetCellRects make_asset_cell_rects(ImVec2 cell_pos,
                                             ImVec2 cell_size) const;
        void draw_asset_cell(AssetEntry &entry, i32 idx,
                             const AssetCellRects &rects);
        void draw_asset_visual(AssetEntry &entry, const AssetCellRects &rects,
                               i32 idx);
        void draw_asset_label(AssetEntry &entry, const AssetCellRects &rects,
                              i32 idx);
        void draw_asset_tooltip(AssetEntry &entry);
        void draw_asset_drag_source(AssetEntry &entry);
        void draw_grid();
        void draw_entry_context_menu(i32 idx);
        void draw_asset_option_menu();
        void draw_empty_space_context_menu();
        void handle_external_drop_target();
        void open_asset(AssetEntry &entry);
        void request_texture_thumbnail(AssetEntry &entry);
        void begin_rename(i32 idx);
        void commit_rename();
        UUID get_asset_uuid(AssetEntry &entry);
        Path get_project_asset_path(const AssetEntry &entry) const;
        std::vector<Path> list_child_directories(const Path &dir);
        bool matches_search(const Path &path, KStr filter);
        Inspectable *create_inspectable(AssetEntry &entry);
        AssetType classify(const Path &p);
        const char *asset_type_icon(AssetType t);
        ImVec4 asset_type_color(AssetType t);
};

}  // namespace Seed

#endif
