#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/rendering/mesh.h"

namespace Seed {
class Terrain : RefCounted {
   private:
    Mesh terrain_mesh;

   public:
    Terrain(Mesh terrain_mesh);
    ~Terrain();
};

}  // namespace Seed

#endif