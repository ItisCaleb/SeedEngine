#ifndef _SEED_VERTEX_LAYOUT_H_
#define _SEED_VERTEX_LAYOUT_H_
#include "core/types.h"
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/math/mat3.h"
#include "core/math/mat4.h"
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
        template <typename T>
        void add_type_attr(u8 layout_num, u32 instance_step,
                           bool should_normalized = false);

        template <>
        void add_type_attr<i32>(u8 layout_num, u32 instance_step,
                                bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::INT, 1,
                           instance_step, should_normalized);
        }
        template <>
        void add_type_attr<u32>(u8 layout_num, u32 instance_step,
                                bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::UNSIGNED, 1,
                           instance_step, should_normalized);
        }
        template <>
        void add_type_attr<f32>(u8 layout_num, u32 instance_step,
                                bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 1,
                           instance_step, should_normalized);
        }

        template <>
        void add_type_attr<Vec2>(u8 layout_num, u32 instance_step,
                                 bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 2,
                           instance_step, should_normalized);
        }

        template <>
        void add_type_attr<Vec3>(u8 layout_num, u32 instance_step,
                                 bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 3,
                           instance_step, should_normalized);
        }

        template <>
        void add_type_attr<Vec4>(u8 layout_num, u32 instance_step,
                                 bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 4,
                           instance_step, should_normalized);
        }

        template <>
        void add_type_attr<Mat3>(u8 layout_num, u32 instance_step,
                                 bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 3,
                           instance_step, should_normalized);
            this->add_attr(layout_num + 1, VertexAttributeType::FLOAT, 3,
                           instance_step, should_normalized);
            this->add_attr(layout_num + 2, VertexAttributeType::FLOAT, 3,
                           instance_step, should_normalized);
        }

        template <>
        void add_type_attr<Mat4>(u8 layout_num, u32 instance_step,
                                 bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 4,
                           instance_step, should_normalized);
            this->add_attr(layout_num + 1, VertexAttributeType::FLOAT, 4,
                           instance_step, should_normalized);
            this->add_attr(layout_num + 2, VertexAttributeType::FLOAT, 4,
                           instance_step, should_normalized);
            this->add_attr(layout_num + 3, VertexAttributeType::FLOAT, 4,
                           instance_step, should_normalized);
        }
        std::vector<VertexAttribute> &get_attrs() { return this->attrs; }
        u32 get_stride() { return this->stride; }
        VertexLayout() = default;
        ~VertexLayout() = default;
};

}  // namespace Seed
#endif