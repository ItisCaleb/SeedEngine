#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include <seed/math/vec3.h>
#include <seed/rendering/mesh.h>

namespace Seed {
class Entity {
   private:
    u32 id;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Mesh *model;

   public:
    Entity(Vec3 position, Vec3 rotation, Vec3 scale, Mesh *model);
    ~Entity();
};

}  // namespace Seed

#endif