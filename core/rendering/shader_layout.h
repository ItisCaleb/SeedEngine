#ifndef _SEED_SHADER_LAYOUT_H_
#define _SEED_SHADER_LAYOUT_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "core/types.h"

namespace Seed {

enum class ShaderResourceType : u8 { UBO, SSBO, SAMPLER };

struct ShaderBinding {
        i64 binding_point;
        ShaderResourceType type;
        i64 count;
        u64 size;
        std::string name;
};

struct ShaderBindingSet {
        std::vector<ShaderBinding> bindings;
};

struct BufferMember {
        u32 offset;
        u32 size;
};

struct PushConstantRange {
        u8 offset;
        u8 size;
};

struct UBOBinding {
        i32 binding = -1;
        std::unordered_map<std::string, BufferMember> members;
        u32 total_size = 0;
};

class ShaderProxy;
class ShaderLayout {
        friend ShaderProxy;

    private:
        std::unordered_map<std::string, i32> texture_unit;
        std::vector<ShaderBindingSet> sets;
        std::vector<PushConstantRange> push_constants;
        UBOBinding ubo_binding;

    public:
        ShaderLayout() = default;
        const std::vector<ShaderBindingSet> &get_binding_sets() const {
            return sets;
        }
        const std::vector<PushConstantRange> &get_push_contants() const {
            return push_constants;
        }

        const UBOBinding &get_ubo_binding() const { return ubo_binding; }

        /* -1 if not found */
        i32 get_texture_unit(const std::string &name) {
            auto iter = texture_unit.find(name);
            if (iter != texture_unit.end()) {
                return iter->second;
            }
            return -1;
        }
};
}  // namespace Seed

#endif