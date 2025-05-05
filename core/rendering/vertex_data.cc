#include "vertex_data.h"

namespace Seed {
VertexData::VertexData(u32 stride,
                       u32 vertex_cnt,const void *data)
    {
    this->vertices_cnt = vertex_cnt;
    this->stride = stride;
    this->vertices.alloc_vertex(stride, vertex_cnt, data);
}

VertexData::VertexData(u32 stride, u32 vertex_cnt,
                       const void *data,const std::vector<u32> &indices)
    : VertexData(stride, vertex_cnt, data) {
    this->use_index = true;
    this->indices_cnt = indices.size();
    this->indices.alloc_index(indices);
}

VertexData::~VertexData() {
    this->vertices.dealloc();
    this->indices.dealloc();
}
}  // namespace Seed