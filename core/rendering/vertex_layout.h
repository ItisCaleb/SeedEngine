#ifndef _SEED_VERTEX_LAYOUT_H_
#define _SEED_VERTEX_LAYOUT_H_
#include "core/types.h"
#include <vector>

namespace Seed {
enum class VertexAttributeType { FLOAT, INT, UNSIGNED, UNSIGNED_BYTE };

struct VertexAttribute {
        u8 layout_num;
        VertexAttributeType type = VertexAttributeType::FLOAT;
        u32 size;
        u32 instance_step;
        bool should_normalized = false;
};

class VertexLayout {
    private:
        std::vector<VertexAttribute> attrs;
        u32 stride = 0;

    public:
        void add_attr(u8 layout_num, VertexAttributeType type, u32 size,
                      u32 instance_step, bool should_normalized = false);
        std::vector<VertexAttribute> &get_attrs() { return this->attrs; }
        u32 get_stride() { return this->stride; }
        VertexLayout() = default;
        ~VertexLayout() = default;
};

}  // namespace Seed
#endif