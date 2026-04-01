#ifndef _SEED_COMPONENTS_H_
#define _SEED_COMPONENTS_H_
#include <tuple>
#include "core/physic/physic_body.h"
#include "core/physic/physic_shape.h"
#include "core/ref.h"
#include "core/resource/model.h"
namespace Seed {

template <typename T>
struct ComponentTrait {};

/* check is create_args is specified */
template <typename T, typename = void>
struct has_create_args : std::false_type {};

template <typename T>
struct has_create_args<T, std::void_t<typename ComponentTrait<T>::create_args>>
    : std::true_type {};

template <typename T>
constexpr bool has_create_args_v = has_create_args<T>::value;

template <>
struct ComponentTrait<PhysicBody> {
        using create_args =
            std::tuple<const PhysicShape &, const PhysicBodyType>;
};

struct MeshInstance {
        Ref<BasicModel> model;
};

struct SkeletonMeshInstance {
        Ref<SkeletonModel> model;
};



}  // namespace Seed

#endif