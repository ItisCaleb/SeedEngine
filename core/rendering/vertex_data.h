#ifndef _SEED_VERTEX_DATA_H_
#define _SEED_VERTEX_DATA_H_
#include "core/rendering/api/render_resource.h"
#include "core/rendering/vertex_layout.h"
#include "core/ref.h"
#include <spdlog/spdlog.h>

namespace Seed {

class VertexData : public RefCounted {
    private:
        u32 count = 0;
        RenderResource vertices;
        VertexLayout *layout = nullptr;
        UpdateFrequence frequence;

    public:
        VertexData(VertexLayout *layout,
                   UpdateFrequence frequence = UpdateFrequence::IMMUTABLE) {
            if (!layout) throw std::runtime_error("Layout is null.");
            this->layout = layout;
            this->frequence = frequence;
            this->vertices.alloc_vertex(layout->get_stride(), 0, frequence,
                                        nullptr);
        }

        template <typename T>
        VertexData(VertexLayout *layout, u32 count, const T *data,
                   UpdateFrequence frequence = UpdateFrequence::IMMUTABLE) {
            if (!layout) throw std::runtime_error("Layout is null.");
            if (layout->get_stride() != sizeof(T)) {
                SPDLOG_ERROR(
                    "Vertex layout doesn't match. layout size = {}, vertex "
                    "size = {}",
                    layout->get_stride(), sizeof(T));
                return;
            }
            this->layout = layout;
            this->count = count;
            this->frequence = frequence;
            this->vertices.alloc_vertex(layout->get_stride(), count, frequence,
                                        (void *)data);
        }

        template <typename T>
        VertexData(VertexLayout *layout, const std::vector<T> &data,
                   UpdateFrequence frequence = UpdateFrequence::IMMUTABLE)
            : VertexData(layout, data.size(), data.data(), frequence) {}
        ~VertexData();

        RenderResource get_resource() { return this->vertices; }
        VertexLayout *get_layout() { return this->layout; }

        u32 get_count() { return count; }

        void _update(u32 size, void *data);

        template <typename T>
        void update(u32 count, const T *data) {
            if (frequence == UpdateFrequence::IMMUTABLE) {
                SPDLOG_ERROR("Cannot update, vertex is immutable");
                return;
            }
            if (layout->get_stride() != sizeof(T)) {
                SPDLOG_ERROR(
                    "Vertex layout doesn't match. layout size = {}, vertex "
                    "size = {}",
                    layout->get_stride(), sizeof(T));
                return;
            }
            this->count = count;
            _update(count * sizeof(T), (void *)data);
        }

        template <typename T>
        void update(const std::vector<T> &data) {
            this->update(data.size(), data.data());
        }
};

enum class IndexType { UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT };
inline static u32 get_index_size(IndexType type) {
    switch (type) {
        case IndexType::UNSIGNED_BYTE:
            return 1;
        case IndexType::UNSIGNED_SHORT:
            return 2;
        case IndexType::UNSIGNED_INT:
        default:
            return 4;
    }
}

class IndexData : public RefCounted {
    private:
        IndexType type = IndexType::UNSIGNED_INT;
        u32 size = 0;
        RenderResource indices;
        UpdateFrequence frequence;

    public:
        IndexType get_type() { return type; }
        RenderResource get_resource() { return this->indices; }

        IndexData(const std::vector<u32> &indices, UpdateFrequence frequence);
        IndexData(const std::vector<u16> &indices, UpdateFrequence frequence);
        IndexData(const std::vector<u8> &indices, UpdateFrequence frequence);

        ~IndexData();
        u32 get_size() { return size; }
        void update(const std::vector<u32> &indices);
        void update(const std::vector<u16> &indices);
        void update(const std::vector<u8> &indices);
};
}  // namespace Seed

#endif