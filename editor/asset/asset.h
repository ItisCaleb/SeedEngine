#ifndef _SEED_ASSET_H_
#define _SEED_ASSET_H_

#include "core/io/path.h"
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

}  // namespace Seed

#endif