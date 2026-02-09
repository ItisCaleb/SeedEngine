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

    public:
        VertexData(VertexLayout *layout) {
            if (!layout) throw std::runtime_error("Layout is null.");
            this->layout = layout;
            this->vertices.alloc_vertex(layout->get_stride(), 0, nullptr);
        }

        template <typename T>
        VertexData(VertexLayout *layout, u32 count, const T *data) {
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
            this->vertices.alloc_vertex(layout->get_stride(), count,
                                        (void *)data);
        }

        template <typename T>
        VertexData(VertexLayout *layout, const std::vector<T> &data)
            : VertexData(layout, data.size(), data.data()) {}
        ~VertexData();

        RenderResource get_resource() { return this->vertices; }
        VertexLayout *get_layout() { return this->layout; }

        u32 get_count() { return count; }

        void _update(u32 size, void* data);

        template <typename T>
        void update(u32 count, const T *data) {
            if (layout->get_stride() != sizeof(T)) {
                SPDLOG_ERROR(
                    "Vertex layout doesn't match. layout size = {}, vertex "
                    "size = {}",
                    layout->get_stride(), sizeof(T));
                return;
            }
            this->count = count;
            _update(count * sizeof(T), (void*)data);
        }

        template <typename T>
        void update(const std::vector<T> &data) {
            this->update(data.size(), data.data());
        }
};

enum class IndexType { UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT };

class IndexData : public RefCounted {
    private:
        IndexType type = IndexType::UNSIGNED_INT;
        u32 size = 0;
        RenderResource indices;

    public:
        IndexType get_type() { return type; }
        RenderResource get_resource() { return this->indices; }

        IndexData(const std::vector<u32> &indices);
        IndexData(const std::vector<u16> &indices);
        IndexData(const std::vector<u8> &indices);

        ~IndexData();
        u32 get_size() { return size; }
        void update(const std::vector<u32> &indices);
        void update(const std::vector<u16> &indices);
        void update(const std::vector<u8> &indices);
};
}  // namespace Seed

#endif