#ifndef _SEED_VERTEX_DATA_H_
#define _SEED_VERTEX_DATA_H_
#include "core/rendering/api/render_resource.h"

namespace Seed {
class VertexData {
    private:
        u32 vertices_cnt;
        u32 stride;
        RenderResource vertices;

        u32 indices_cnt;
        RenderResource indices;

        bool indexing = false;

    public:
        VertexData(){}
        VertexData(u32 stride, u32 vertex_cnt, const void *data);
        VertexData(u32 stride, u32 vertex_cnt, const void *data,
                   const std::vector<u32> &indices);

        ~VertexData();

        void bind_vertices(u32 stride, u32 vertex_cnt, RenderResource vertices);
        RenderResource *get_vertices() { return &this->vertices; }

        RenderResource *get_indices() { return &this->indices; }

        u32 get_vertices_cnt() { return vertices_cnt; }

        u32 get_indices_cnt() { return indices_cnt; }

        u32 get_stride() { return stride; }

        bool use_index() { return indexing; }
};
}  // namespace Seed

#endif