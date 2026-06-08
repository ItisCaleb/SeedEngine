#ifndef _SEED_ASSET_H_
#define _SEED_ASSET_H_

#include <nlohmann/detail/macro_scope.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
#include "core/types.h"
#include "editor/gui/inspectable.h"
#include "editor/gui/popup.h"
namespace Seed {
enum class AssetType {
    Unknown,
    Texture,
    Mesh,
    World,
    Material,
    Audio,
    Script,
    Directory,
};

struct AssetEntry {
        Path path;
        AssetType type;
        // thumbnail texture handle — 0 if not loaded yet
        u64 thumbnail_handle = 0;
};

class ModelInspector : public Inspectable {
    private:
        struct Material {
                KString name;
                UUID diffuse;
                UUID specular;
                UUID normal;
                f32 opacity;
        };

        std::vector<Material> materials;

    public:
        ModelInspector(ResourceConfiguration &config);
        virtual void draw_inspector() override;
        virtual void save() override;
};


class WorldCreatePopup : public Popup {
    private:
        char new_world_name[256] = {};
        void create_world();

    public:
        virtual void draw() override;
};

}  // namespace Seed

#endif
