#ifndef _SEED_VERTEX_DATA_H_
#define _SEED_VERTEX_DATA_H_
#include "core/rendering/api/render_resource.h"

namespace Seed {

enum class IndexType { UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT };
class VertexData {
    private:
        u32 vertices_cnt;
        u32 stride;
        RenderResource vertices;

        IndexType index_type = IndexType::UNSIGNED_INT;
        u32 indices_cnt;
        RenderResource indices;

        bool indexing = false;

    public:
        VertexData() {}
        VertexData(u32 stride, u32 vertex_cnt, const void *data);
        VertexData(u32 stride, u32 vertex_cnt, const void *data,
                   const std::vector<u8> &indices);
        VertexData(u32 stride, u32 vertex_cnt, const void *data,
                   const std::vector<u16> &indices);
        VertexData(u32 stride, u32 vertex_cnt, const void *data,
                   const std::vector<u32> &indices);

        ~VertexData();

        void bind_vertices(u32 stride, u32 vertex_cnt, RenderResource vertices);
        RenderResource *get_vertices() { return &this->vertices; }

        RenderResource *get_indices() { return &this->indices; }

        u32 get_vertices_cnt() { return vertices_cnt; }

        u32 get_indices_cnt() { return indices_cnt; }

        u32 get_stride() { return stride; }

        IndexType get_index_type() { return index_type; }

        bool use_index() { return indexing; }

        void alloc_vertex(u32 stride, u32 vertex_cnt, const void *data);
        void alloc_index(const std::vector<u32> &indices);
        void alloc_index(const std::vector<u16> &indices);
        void alloc_index(const std::vector<u8> &indices);

        void set_vertices_cnt(u32 cnt) { this->vertices_cnt = cnt; };
        void set_indices_cnt(u32 cnt) { this->indices_cnt = cnt; };
};
}  // namespace Seed

#endif