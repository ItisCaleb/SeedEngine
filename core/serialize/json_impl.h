#ifndef _SEED_JSON_IMPL_H_
#define _SEED_JSON_IMPL_H_
#include <nlohmann/json.hpp>
#include "core/types.h"
#include "core/math/vec3.h"
#include "core/collision/shape.h"

namespace Seed {

template <typename json_type>
inline void to_json(json_type &j, const Seed::Vec3 &v) {
    j = json_type{v.x, v.y, v.z};
}

template <typename json_type>
inline void to_json(json_type &j, const Seed::AABB &aabb) {
    j = json_type{{"center", aabb.center}, {"ext", aabb.ext}};
}

}  // namespace Seed

#endif