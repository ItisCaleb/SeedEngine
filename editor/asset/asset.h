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
        bool is_dir;
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

struct WorldConfig {
        KString name;
        u32 width, height;
        UUID height_map;
        UUID splat_map;
        UUID light_map;
        struct Sky {
                UUID up;
                UUID down;
                UUID left;
                UUID right;
                UUID front;
                UUID back;
        } sky;
        UUID texture1;
        UUID texture1_normal;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorldConfig::Sky, up, down, left, right,
                                   front, back);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorldConfig, name, width, height, sky,
                                   texture1, texture1_normal);

class WorldCreatePopup : public Popup {
    private:
        char new_terrain_name[256] = {};
        i32 new_terrain_w = 0, new_terrain_h = 0;
        bool load_from_heightmap;
    public:
        virtual void draw() override;
};

class WorldInspector : public Inspectable {
    private:
        WorldConfig world;

    public:
        WorldInspector(ResourceConfiguration &config);
        virtual void draw_inspector() override;
        virtual void save() override;
};

}  // namespace Seed

#endif