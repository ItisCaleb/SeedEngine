#ifndef _SEED_JOLT_BACKEND_H_
#define _SEED_JOLT_BACKEND_H_
#ifndef JPH_DEBUG_RENDERER
#define JPH_DEBUG_RENDERER
#endif

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include "core/handle.h"
#include "core/physic/physic_shape.h"
#include "core/physic/physic_body.h"
#include "core/math/vec3.h"
#include "core/math/quaternion.h"

namespace Seed {

namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr u32 NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            // Create a mapping table from object to broad phase layer
            mObjectToBroadPhase[Layers::NON_MOVING] =
                BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual u32 GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(
            JPH::ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl
    : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(
            JPH::ObjectLayer inLayer1,
            JPH::BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
                case Layers::NON_MOVING:
                    return inLayer2 == BroadPhaseLayers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                                   JPH::ObjectLayer inObject2) const override {
            switch (inObject1) {
                case Layers::NON_MOVING:
                    return inObject2 == Layers::MOVING;  // Non moving only
                                                         // collides with moving
                case Layers::MOVING:
                    return true;  // Moving collides with everything
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
};

class JoltJobWrapper : public JPH::JobSystem {};

class JoltDebugRenderer : public JPH::DebugRenderer {
    public:
        JPH_OVERRIDE_NEW_DELETE
        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo,
                      JPH::ColorArg inColor) override;
        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2,
                          JPH::RVec3Arg inV3, JPH::ColorArg inColor,
                          ECastShadow inCastShadow = ECastShadow::Off) override;
        Batch CreateTriangleBatch(const Triangle *inTriangles,
                                  int inTriangleCount) override;
        Batch CreateTriangleBatch(const Vertex *inVertices, int inVertexCount,
                                  const uint32_t *inIndices,
                                  int inIndexCount) override;
        void DrawGeometry(JPH::RMat44Arg inModelMatrix,
                          const JPH::AABox &inWorldSpaceBounds,
                          float inLODScaleSq, JPH::ColorArg inModelColor,
                          const GeometryRef &inGeometry,
                          ECullMode inCullMode = ECullMode::CullBackFace,
                          ECastShadow inCastShadow = ECastShadow::On,
                          EDrawMode inDrawMode = EDrawMode::Solid) override;

        void DrawText3D(JPH::RVec3Arg inPosition,
                        const std::string_view &inString,
                        JPH::ColorArg inColor = JPH::Color::sWhite,
                        float inHeight = 0.5f) override;
        JoltDebugRenderer();
};

class JoltBackend {
        static void trace(const char *inFMT, ...);

    private:
        HandleOwner<JPH::BodyID> bodys;
        JPH::PhysicsSystem system;
        JPH::JobSystemThreadPool *job_system;
        JPH::TempAllocatorImpl *temp_allocator;
        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        JPH::ShapeRefC create_shape(PhysicShape &shape);

    public:
        JoltBackend();
        void process();
        void create_body(PhysicBody &body, PhysicShape &shape,
                         PhysicBodyType type, Vec3 &pos,
                         const Quaternion &quat);
        void delete_body(PhysicBody &body);
        inline void query_position(PhysicBody &body, Vec3 &position);
        inline void query_rotation(PhysicBody &body, Quaternion &quat);
        void query_physics(PhysicBody &body, Vec3 &position, Quaternion &quat);
};

}  // namespace Seed

#endif