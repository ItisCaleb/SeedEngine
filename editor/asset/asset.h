#ifndef _SEED_ASSET_H_
#define _SEED_ASSET_H_

#include <nlohmann/detail/macro_scope.hpp>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/resource/resource_entry.h"
#include "core/resource/texture.h"
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
        Ref<Texture> thumbnail_texture;
        u32 texture_width = 0;
        u32 texture_height = 0;
        bool thumbnail_requested = false;
        bool thumbnail_failed = false;
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
