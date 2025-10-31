#include "vertex_data.h"
#include "core/rendering/api/render_command.h"

namespace Seed {

VertexData::~VertexData() { this->vertices.dealloc(); }

IndexData::IndexData(const std::vector<u8> &indices) {
    this->indices.alloc_index(indices);
    this->type = IndexType::UNSIGNED_BYTE;
    this->size = indices.size();
}
IndexData::IndexData(const std::vector<u16> &indices) {
    this->indices.alloc_index(indices);
    this->type = IndexType::UNSIGNED_SHORT;
    this->size = indices.size();
}
IndexData::IndexData(const std::vector<u32> &indices) {
    this->indices.alloc_index(indices);
    this->type = IndexType::UNSIGNED_INT;
    this->size = indices.size();
}

void IndexData::update(const std::vector<u8> &indices) {
    if (type != IndexType::UNSIGNED_BYTE) {
        SPDLOG_ERROR("Index Type doens't match. Provided: u8");
        return;
    }
    RenderCommandDispatcher dp;
    dp.update_buffer(this->indices, 0, sizeof(u8) * indices.size(),
                     (void *)indices.data());
    this->size = indices.size();
}
void IndexData::update(const std::vector<u16> &indices) {
    if (type != IndexType::UNSIGNED_SHORT) {
        SPDLOG_ERROR("Index Type doens't match. Provided: u16");
        return;
    }
    RenderCommandDispatcher dp;
    dp.update_buffer(this->indices, 0, sizeof(u16) * indices.size(),
                     (void *)indices.data());
    this->size = indices.size();
}
void IndexData::update(const std::vector<u32> &indices) {
    if (type != IndexType::UNSIGNED_INT) {
        SPDLOG_ERROR("Index Type doens't match. Provided: u32");
        return;
    }
    RenderCommandDispatcher dp;
    dp.update_buffer(this->indices, 0, sizeof(u32) * indices.size(),
                     (void *)indices.data());
    this->size = indices.size();
}

IndexData::~IndexData() { this->indices.dealloc(); }

}  // namespace Seed