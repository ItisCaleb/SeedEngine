#ifndef _SEED_VERTEX_DATA_H_
#define _SEED_VERTEX_DATA_H_
#include "core/ref.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_desc.h"

namespace Seed {
class VertexData {
   private:
    u32 vertices_cnt;
    u32 stride;
    RenderResource vertices;

    u32 indices_cnt;
    RenderResource indices;

    VertexDescription *desc;
    bool use_index = false;
   public:
    VertexData(VertexDescription *desc, u32 vertex_cnt, u32 stride, void *vertex_data);
    VertexData(VertexDescription *desc, u32 vertex_cnt, u32 stride, void *vertex_data, std::vector<u32> &indices);

    ~VertexData();
    RenderResource* get_vertices(){
        return &this->vertices;
    }

    RenderResource* get_indices(){
        return &this->vertices;
    }
    
};
}  // namespace Seed

#endif