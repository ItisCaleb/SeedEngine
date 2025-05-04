#include "vertex_data.h"

namespace Seed {
VertexData::VertexData(VertexDescription *desc, u32 vertex_cnt,
                       u32 vertex_stride, void *vertex_data)
    : desc(desc) {
    this->vertices_cnt = vertex_cnt;
    this->stride = vertex_stride;
    this->vertices.alloc_vertex(this->stride, vertex_cnt, vertex_data);
}

VertexData::VertexData(VertexDescription *desc, u32 vertex_cnt, u32 stride,
                       void *vertex_data, std::vector<u32> &indices)
    : VertexData(desc, vertex_cnt, stride, vertex_data) {
    this->use_index = true;
    this->indices.alloc_index(indices);
}

VertexData::~VertexData() {
    this->vertices.dealloc();
    this->indices.dealloc();
}
}  // namespace Seed