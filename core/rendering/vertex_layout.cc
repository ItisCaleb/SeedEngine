#include "vertex_layout.h"

namespace Seed {
void VertexLayout::add_attr(u8 layout_num, VertexAttributeType type, u32 size,
                            bool should_normalized) {
    this->attrs.push_back({.layout_num = layout_num,
                           .type = type,
                           .size = size,
                           .should_normalized = should_normalized});

    this->stride += size * this->attrs.back().get_type_size();
}
}  // namespace Seed