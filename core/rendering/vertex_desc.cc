#include "vertex_desc.h"

namespace Seed {
void VertexDescription::add_attr(u8 layout_num, VertexAttributeType type,
                                 u32 size, u32 instance_step,
                                 bool should_normalized) {
    this->attrs.push_back({.layout_num = layout_num,
                           .type = type,
                           .size = size,
                           .instance_step = instance_step,
                           .should_normalized = should_normalized});
    
    u32 type_size = 1;
    switch (type)
    {
    case VertexAttributeType::FLOAT:
    case VertexAttributeType::INT:
    case VertexAttributeType::UNSIGNED:
    default:
        type_size = 4;
        break;
    }
    this->stride += size * type_size;
}
}  // namespace Seed