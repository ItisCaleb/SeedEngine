#ifndef _SEED_OBJECT_H_
#define _SEED_OBJECT_H_
#include "math/vec3.h"
#include "rendering/mesh.h"

namespace Seed {
class Entity {
   private:
    u32 id;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Ref<Mesh> model;

   public:
    Ref<Mesh> get_mesh();
    void bind_mesh(Ref<Mesh> model);
    Vec3 get_position();
    void set_position(Vec3 position);
    Vec3 get_rotation();
    void set_rotation(Vec3 position);
    Vec3 get_scale();
    void set_scale(Vec3 position);

    virtual void update(f32 dt){}

    Entity(Vec3 position, Vec3 rotation, Vec3 scale, Ref<Mesh> model);
    Entity(Vec3 position, Ref<Mesh> model);
    Entity();
    ~Entity();
};

}  // namespace Seed

#endif