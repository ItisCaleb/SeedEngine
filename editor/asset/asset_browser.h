#ifndef _SEED_ASSET_BROWSER_H_
#define _SEED_ASSET_BROWSER_H_

#include <imgui.h>
#include <map>
#include <mutex>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/math/vec2.h"
#include "core/resource/image.h"
#include "core/types.h"
#include "editor/asset/asset.h"
#include "editor/gui/inspectable.h"

namespace Seed {

class AssetBrowser {
    public:
        void init(KStr project_root);
        void update();  // call inside your panel
        Ref<Dir> get_current_dir() { return current_dir; }

    private:
        // State
        Ref<Dir> root_dir;
        Ref<Dir> current_dir;
        std::vector<AssetEntry> entries;  // contents of current_dir
        std::map<Path, std::vector<AssetEntry>> folder_entry_cache;
        i32 selected_idx = -1;
        f32 folder_panel_width = 220.f;
        f32 icon_size = 72.f;
        char search_buf[128] = {};
        bool needs_refresh = true;

        // Rename state
        int renaming_idx = -1;
        char rename_buf[256] = {};
        bool show_popup;

        // Breadcrumb
        std::vector<Path> breadcrumbs;  // root → current_dir

        struct ThumbnailResult {
                Path path;
                Ref<Image> image;
                u32 source_width = 0;
                u32 source_height = 0;
                bool failed = false;
        };

        std::mutex thumbnail_results_mutex;
        std::vector<ThumbnailResult> thumbnail_results;

        // Helpers
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
        void draw_icon(AssetEntry &e, Vec2 cell_size, u32 rename_idx);
        void draw_grid();
        void draw_entry_context_menu(int idx);
        void draw_asset_option_menu();
        void draw_empty_space_context_menu();
        void handle_external_drop_target();
        void open_asset(AssetEntry &entry);
        void request_texture_thumbnail(AssetEntry &entry);
        void begin_rename(int idx);
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
