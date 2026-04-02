#include "human_entity.h"
#include "core/input.h"
#include "core/engine.h"
#include <fmt/core.h>
#include "core/math/utils.h"
#include "core/ref.h"
#include "core/resource/animation.h"
#include "core/resource/model.h"
#include "core/transform.h"
#include "core/world/components.h"
#include "core/world/entity.h"

namespace Seed {

HumanBehaviour::HumanBehaviour(Ref<SkeletonModel> model, AnimationState *state)
    : model(model), state(state) {
    if(model->get_animations().size() > 0){
        this->state->set_animation(model->get_animations()[0]);
    }
}
void HumanBehaviour::start() {}

void HumanBehaviour::update(f32 dt) {}
Entity HumanEntity::create_entity(EntityManager &m, Transform &transform,
                                  Ref<SkeletonModel> model) {
    Entity e = m.create_entity();
    Ref<HumanBehaviour> b;
    m.add_component<Transform>(e, transform);
    m.add_component<SkeletonMeshInstance>(e,
                                          SkeletonMeshInstance{.model = model});
    AnimationState *state = m.add_component<AnimationState>(e);
    b.create(model, state);

    m.add_component<BehaviourComponent>(
        e, BehaviourComponent{.behaviour = ref_cast<Behaviour>(b)});
    return e;
}
void HumanEntity::destroy_entity(EntityManager &m, Entity e) {
    m.remove_component<Transform>(e);
    m.remove_component<SkeletonMeshInstance>(e);
    m.remove_component<AnimationState>(e);
    m.remove_component<BehaviourComponent>(e);
    m.destroy_entity(e);
}
}  // namespace Seed
