#ifndef _SEED_JSON_IMPL_H_
#define _SEED_JSON_IMPL_H_
#include <nlohmann/json.hpp>
#include <string>
#include "core/container/kstring.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/math/mat4.h"
#include "core/collision/shape.h"
#include "core/io/path.h"

namespace Seed {

template <typename json_type>
inline void to_json(json_type &j, const Vec3 &v) {
    j = json_type{v.x, v.y, v.z};
}

template <typename json_type>
inline void from_json(json_type &j, Vec3 &v) {
    v.x = j[0];
    v.y = j[1];
    v.z = j[2];
}

template <typename json_type>
inline void to_json(json_type &j, const Vec4 &v) {
    j = json_type{v.x, v.y, v.z, v.w};
}

template <typename json_type>
inline void from_json(json_type &j, Vec4 &v) {
    v.x = j[0];
    v.y = j[1];
    v.z = j[2];
    v.w = j[3];
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AABB, center, ext);

template <typename json_type>
inline void to_json(json_type &j, const Mat4 &m) {
    j = json_type{m[0], m[1], m[2], m[3]};
}

template <typename json_type>
inline void from_json(json_type &j, KString &v) {
    const std::string &s = j.template get<std::string>();
    v = s;
}

template <typename json_type>
inline void to_json(json_type &j, const KString &v) {
    j = v.to_str().data();
}

template <typename json_type>
inline void from_json(json_type &j, Path &v) {
    const std::string &s = j.template get<std::string>();
    v = s;
}

template <typename json_type>
inline void to_json(json_type &j, const Path &v) {
    j = v.to_str().data();
}

}  // namespace Seed

#endif