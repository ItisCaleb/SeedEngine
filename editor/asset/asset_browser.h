#ifndef _SEED_ASSET_BROWSER_H_
#define _SEED_ASSET_BROWSER_H_

#include <string>
#include <RmlUi/Core/DataModelHandle.h>
#include <map>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/types.h"
#include "editor/asset/asset.h"
#include "core/gui/gui.h"

namespace Seed {

class AssetBrowser : public RmlGUI {
    public:
        AssetBrowser();
        void init(const Path &project_root);
        Ref<Dir> get_current_dir() { return current_dir; }

    private:
        struct AssetItemView {
                UUID uuid;
                KString name;
                KString type;
                KString icon;
                i32 entry_index = -1;
        };

        struct AssetFolderView {
                KString name;
                bool active = false;
        };

        Ref<Dir> root_dir;
        Ref<Dir> current_dir;
        std::vector<AssetEntry> entries;
        std::vector<AssetItemView> asset_items;
        std::vector<AssetFolderView> folder_items;
        std::vector<Path> folder_paths;
        std::string search_text;
        std::string new_world_name;
        std::string new_world_path;
        std::string world_create_error;
        KString breadcrumb_text;
        Rml::DataModelHandle asset_model;
        Rml::DataModelHandle world_create_model;
        std::map<Path, std::vector<AssetEntry>> folder_entry_cache;
        i32 selected_idx = -1;
        char search_buf[128] = {};
        bool needs_refresh = true;
        i32 renaming_idx = -1;
        char rename_buf[256] = {};
        std::vector<Path> breadcrumbs;

        void refresh();
        void sync_view_model();
        void dirty_view_model();
        void invalidate_current_folder_cache();
        void navigate_to(const Path &dir);

        void open_asset(AssetEntry &entry);
        void begin_rename(i32 idx);
        void commit_rename();
        UUID get_asset_uuid(AssetEntry &entry);
        Path get_project_asset_path(const AssetEntry &entry) const;
        std::vector<Path> list_child_directories(const Path &dir);
        bool matches_search(const Path &path, KStr filter);
        AssetType classify(const Path &p);
        const char *asset_type_icon(AssetType t);
        const char *asset_type_name(AssetType t);
        bool show_world_create = false;
        bool has_world_create_error = false;
        Path current_asset_directory() const;
        bool create_world_asset();
        void rml_refresh(RML_EVENT_ARGS);
        void rml_open_asset(RML_EVENT_ARGS);
        void rml_open_folder(RML_EVENT_ARGS);
        void rml_open_menu(RML_EVENT_ARGS);
        void rml_request_create_world(RML_EVENT_ARGS);
        void rml_cancel_create_world(RML_EVENT_ARGS);
        void rml_confirm_create_world(RML_EVENT_ARGS);

        void bind_model(Rml::Context *context) override;
};

}  // namespace Seed

#endif
