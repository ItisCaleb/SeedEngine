#ifndef _SEED_ASSET_BROWSER_H_
#define _SEED_ASSET_BROWSER_H_

#include <imgui.h>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/types.h"
#include "editor/asset/asset.h"
#include "editor/gui/inspectable.h"

namespace Seed {

class AssetBrowser {
    public:
        void init(KStr project_root);
        void update();  // call inside your panel

    private:
        // State
        Ref<Dir> root_dir;
        Ref<Dir> current_dir;
        std::vector<AssetEntry> entries;  // contents of current_dir
        i32 selected_idx = -1;
        i32 view_mode = 0;  // 0=grid 1=list
        f32 icon_size = 72.f;
        char search_buf[128] = {};
        bool needs_refresh = true;

        // Rename state
        int renaming_idx = -1;
        char rename_buf[256] = {};

        // Breadcrumb
        std::vector<Path> breadcrumbs;  // root → current_dir

        // Helpers
        void refresh();
        void navigate_to(KStr dir);
        void draw_breadcrumb();
        void draw_toolbar();
        void draw_grid();
        void draw_list();
        void draw_entry_context_menu(int idx);
        void begin_rename(int idx);
        void commit_rename();
        UUID get_asset_uuid(AssetEntry &entry);
        Inspectable *create_inspectable(AssetEntry &entry);
        AssetType classify(const Path &p);
        const char *asset_type_icon(AssetType t);
        ImVec4 asset_type_color(AssetType t);
};

}  // namespace Seed


#endif