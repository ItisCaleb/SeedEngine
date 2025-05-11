#include "vertex_data.h"

namespace Seed {
VertexData::VertexData(u32 stride, u32 vertex_cnt, const void *data) {
    this->vertices_cnt = vertex_cnt;
    this->stride = stride;
    this->vertices.alloc_vertex(stride, vertex_cnt, data);
}

VertexData::VertexData(u32 stride, u32 vertex_cnt, const void *data,
                       const std::vector<u8> &indices)
    : VertexData(stride, vertex_cnt, data) {
    this->alloc_index(indices);
}

VertexData::VertexData(u32 stride, u32 vertex_cnt, const void *data,
                       const std::vector<u16> &indices)
    : VertexData(stride, vertex_cnt, data) {
    this->alloc_index(indices);
}

VertexData::VertexData(u32 stride, u32 vertex_cnt, const void *data,
                       const std::vector<u32> &indices)
    : VertexData(stride, vertex_cnt, data) {
    this->alloc_index(indices);
}

VertexData::~VertexData() {
    this->vertices.dealloc();
    this->indices.dealloc();
}

void VertexData::bind_vertices(u32 stride, u32 vertex_cnt,
                               RenderResource vertices) {
    if (this->vertices.inited() && this->vertices.handle != vertices.handle) {
        this->vertices.dealloc();
    }
    this->vertices = vertices;
    this->stride = stride;
    this->vertices_cnt = vertex_cnt;
}

void VertexData::alloc_vertex(u32 stride, u32 vertex_cnt, const void *data) {
    if (this->vertices.inited()) {
        this->vertices.dealloc();
    }
    this->stride = stride;
    this->vertices_cnt = vertex_cnt;
    this->vertices.alloc_vertex(stride, vertex_cnt, data);
}
void VertexData::alloc_index(const std::vector<u8> &indices) {
    if (this->indices.inited()) {
        this->indices.dealloc();
    }
    this->index_type = IndexType::UNSIGNED_BYTE;
    this->indices_cnt = indices.size();
    this->indices.alloc_index(indices);
    this->indexing = true;
}
void VertexData::alloc_index(const std::vector<u16> &indices) {
    if (this->indices.inited()) {
        this->indices.dealloc();
    }
    this->index_type = IndexType::UNSIGNED_SHORT;
    this->indices_cnt = indices.size();
    this->indices.alloc_index(indices);
    this->indexing = true;
}
void VertexData::alloc_index(const std::vector<u32> &indices) {
    if (this->indices.inited()) {
        this->indices.dealloc();
    }
    this->index_type = IndexType::UNSIGNED_INT;

    this->indices_cnt = indices.size();
    this->indices.alloc_index(indices);
    this->indexing = true;
}
}  // namespace Seed