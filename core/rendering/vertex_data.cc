#include "vertex_data.h"
#include "core/rendering/api/render_command.h"

namespace Seed {

void VertexData::_update(u32 size, void *data) {
    RenderCommandDispatcher dp;
    dp.update_buffer(this->vertices, 0, size, data);
}

VertexData::~VertexData() { this->vertices.dealloc(); }

IndexData::IndexData(const std::vector<u8> &indices,
                     UpdateFrequence frequence) {
    this->indices.alloc_index(indices, frequence);
    this->type = IndexType::UNSIGNED_BYTE;
    this->size = indices.size();
    this->frequence = frequence;
}
IndexData::IndexData(const std::vector<u16> &indices,
                     UpdateFrequence frequence) {
    this->indices.alloc_index(indices, frequence);
    this->type = IndexType::UNSIGNED_SHORT;
    this->size = indices.size();
    this->frequence = frequence;
}
IndexData::IndexData(const std::vector<u32> &indices,
                     UpdateFrequence frequence) {
    this->indices.alloc_index(indices, frequence);
    this->type = IndexType::UNSIGNED_INT;
    this->size = indices.size();
    this->frequence = frequence;
}

void IndexData::update(const std::vector<u8> &indices) {
    if (frequence == UpdateFrequence::IMMUTABLE) {
        SPDLOG_ERROR("Cannot update, index is immutable");
        return;
    }
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
    if (frequence == UpdateFrequence::IMMUTABLE) {
        SPDLOG_ERROR("Cannot update, index is immutable");
        return;
    }
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
    if (frequence == UpdateFrequence::IMMUTABLE) {
        SPDLOG_ERROR("Cannot update, index is immutable");
        return;
    }
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