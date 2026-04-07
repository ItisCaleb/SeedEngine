#ifndef _SEED_ASSET_H_
#define _SEED_ASSET_H_

#include <vector>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource_entry.h"
#include "editor/gui/inspectable.h"
#include "model_loader.h"
namespace Seed {
enum class AssetType {
    Unknown,
    Texture,
    Mesh,
    Terrain,
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
        struct Material {
                KString name;
                UUID diffuse;
                UUID specular;
                UUID normal;
                f32 opacity;
        };

    private:
        std::vector<Material> materials;

    public:
        ModelInspector(ResourceConfiguration &config);
        virtual void draw_inspector() override;
                virtual void save() override;

};

}  // namespace Seed

#endif