#ifndef _SEED_VERTEX_DATA_H_
#define _SEED_VERTEX_DATA_H_
#include "core/ref.h"
#include "core/rendering/api/render_resource.h"

namespace Seed {
class VertexData {
   private:
    RenderResource vertices;
   public:
    VertexData(/* args */);
    ~VertexData();
    RenderResource* get_vertices(){
        return &this->vertices;
    }
};
}  // namespace Seed

#endif