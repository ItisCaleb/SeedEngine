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
enum class VertexAttributeType : u8 {
    FLOAT,
    INT,
    UNSIGNED,
    SHORT,
    USHORT,
    UNSIGNED_BYTE
};

struct VertexAttribute {
        u8 layout_num;
        VertexAttributeType type = VertexAttributeType::FLOAT;
        u32 size;
        bool should_normalized = false;
        u32 get_type_size() const {
            u32 type_size = 1;
            switch (type) {
                case VertexAttributeType::UNSIGNED_BYTE:
                    type_size = 1;
                    break;
                case VertexAttributeType::SHORT:
                case VertexAttributeType::USHORT:
                    type_size = 2;
                    break;
                case VertexAttributeType::FLOAT:
                case VertexAttributeType::INT:
                case VertexAttributeType::UNSIGNED:
                default:
                    type_size = 4;
                    break;
            }
            return type_size;
        }
};

class VertexLayout {
    private:
        std::vector<VertexAttribute> attrs;
        u32 stride = 0;
        bool instance;

    public:
        void add_attr(u8 layout_num, VertexAttributeType type, u32 size,
                      bool should_normalized = false);
        template <typename T>
        void add_type_attr(u8 layout_num, bool should_normalized = false);

        template <>
        void add_type_attr<i32>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::INT, 1,
                           should_normalized);
        }
        template <>
        void add_type_attr<u32>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::UNSIGNED, 1,
                           should_normalized);
        }
        template <>
        void add_type_attr<f32>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 1,
                           should_normalized);
        }

        template <>
        void add_type_attr<Vec2>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 2,
                           should_normalized);
        }

        template <>
        void add_type_attr<Vec3>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 3,
                           should_normalized);
        }

        template <>
        void add_type_attr<Vec4>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 4,
                           should_normalized);
        }

        template <>
        void add_type_attr<Mat3>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 3,
                           should_normalized);
            this->add_attr(layout_num + 1, VertexAttributeType::FLOAT, 3,
                           should_normalized);
            this->add_attr(layout_num + 2, VertexAttributeType::FLOAT, 3,
                           should_normalized);
        }

        template <>
        void add_type_attr<Mat4>(u8 layout_num, bool should_normalized) {
            this->add_attr(layout_num, VertexAttributeType::FLOAT, 4,
                           should_normalized);
            this->add_attr(layout_num + 1, VertexAttributeType::FLOAT, 4,
                           should_normalized);
            this->add_attr(layout_num + 2, VertexAttributeType::FLOAT, 4,
                           should_normalized);
            this->add_attr(layout_num + 3, VertexAttributeType::FLOAT, 4,
                           should_normalized);
        }
        std::vector<VertexAttribute> &get_attrs() { return this->attrs; }
        u32 get_stride() { return this->stride; }
        void set_instance(bool is_instance) { this->instance = is_instance; }
        bool is_instance() { return this->instance; }
        VertexLayout(bool is_instance = false) : instance(is_instance) {};
        ~VertexLayout() = default;
};

}  // namespace Seed
#endif