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
        UUID uuid;
        Path path;
        AssetType type;
};

class Asset {
        static AssetType uuid_to_type(UUID uuid);
};

}  // namespace Seed

#endif
