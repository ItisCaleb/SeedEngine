#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/rendering/mesh.h"

namespace Seed {

struct TerrainVertex{
    Vec3 pos;
    Vec2 tex_coord;
};

class Terrain : public RefCounted {
   private:
    u32 width, depth;
    Ref<Texture> height_map;
   public:
    Terrain(u32 width, u32 depth, Ref<Texture> height_map);
    ~Terrain();
};

}  // namespace Seed

#endif