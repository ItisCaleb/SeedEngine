#ifndef _SEED_SHADER_LAYOUT_H_
#define _SEED_SHADER_LAYOUT_H_

#include <string>
#include <unordered_map>
#include "core/types.h"

namespace Seed {

enum class ShaderResourceType : u8 { UBO, SSBO, SAMPLER };

struct ShaderBinding {
        i64 binding_point;
        ShaderResourceType type;
        i64 count;
        std::string name;
};

struct ShaderBindingSet {
        std::vector<ShaderBinding> bindings;
};
struct PushConstantRange {
        u8 offset;
        u8 size;
};

class ShaderProxy;
class ShaderLayout {
        friend ShaderProxy;

    private:
        std::unordered_map<std::string, i32> texture_unit;
        std::vector<ShaderBindingSet> sets;
        std::vector<PushConstantRange> push_constants;

    public:
        ShaderLayout() = default;
        const std::vector<ShaderBindingSet> &get_binding_sets() const {
            return sets;
        }
        const std::vector<PushConstantRange> &get_push_contants() const {
            return push_constants;
        }

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