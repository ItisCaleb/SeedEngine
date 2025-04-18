#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/rendering/mesh.h"

namespace Seed {
class Terrain : public RefCounted {
   private:
    u32 width, depth;
    RenderResource height_map;
   public:
    Terrain(u32 width, u32 depth, RenderResource height_map);
    ~Terrain();
};

}  // namespace Seed

#endif