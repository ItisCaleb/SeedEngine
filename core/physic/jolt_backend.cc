#include "jolt_backend.h"
#include <thread>
#include <spdlog/spdlog.h>
#include <stdarg.h>
#include "core/debug/debug_drawer.h"
#include "core/macro.h"
#include "core/engine.h"

namespace Seed {

void JoltBackend::trace(const char *inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    SPDLOG_DEBUG("{}", buffer);
}

inline static Vec3 from_jolt(const JPH::Vec3 &from) {
    return Vec3{from.GetX(), from.GetY(), from.GetZ()};
}

inline static JPH::Vec3 to_jolt(const Vec3 &from) {
    return JPH::Vec3{from.x, from.y, from.z};
}

inline static Color from_jolt(const JPH::Color &from) {
    return Color{from.r, from.g, from.b, from.a};
}

inline static JPH::Quat to_jolt(const Quaternion &quat) {
    return JPH::Quat{quat.x, quat.y, quat.z, quat.w};
}

inline static Quaternion from_jolt(const JPH::Quat &from) {
    return Quaternion{from.GetW(), from.GetX(), from.GetY(), from.GetZ()};
}

JoltBackend::JoltBackend() {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = trace;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::DebugRenderer::sInstance = new JoltDebugRenderer;
    JPH::RegisterTypes();
    const u32 cMaxBodies = 65536;
    const u32 cNumBodyMutexes = 0;
    const u32 cMaxBodyPairs = 65536;
    const u32 cMaxContactConstraints = 10240;
    this->job_system = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1);
    this->temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                cMaxContactConstraints, broad_phase_layer_interface,
                object_vs_broadphase_layer_filter,
                object_vs_object_layer_filter);
}

void JoltBackend::process() {
    system.Update(1.0f / 60, 1, temp_allocator, job_system);
}

void JoltBackend::query_position(PhysicBody &body, Vec3 &position) {}
void JoltBackend::query_rotation(PhysicBody &body, Quaternion &quat) {}

void JoltBackend::query_physics(PhysicBody &body, Vec3 &position,
                                Quaternion &quat) {
    JPH::BodyID *body_id = this->bodys.get_or_null(body.handle);
    EXPECT_NOT_NULL_RET(body_id);
    JPH::RVec3 j_pos;
    JPH::Quat j_quat;
    system.GetBodyInterface().GetPositionAndRotation(*body_id, j_pos, j_quat);

    position = from_jolt(j_pos);
    quat = from_jolt(j_quat);
}

JPH::ShapeRefC JoltBackend::create_shape(PhysicShape &shape) {
    switch (shape.type) {
        case PhysicShapeType::BOX: {
            PhysicBoxShape &box_shape = static_cast<PhysicBoxShape &>(shape);
            JPH::BoxShapeSettings setting(to_jolt(box_shape.half_extent));
            return setting.Create().Get();
        }

        default:
            return JPH::ShapeRefC();
    }
}

void JoltBackend::create_body(PhysicBody &body, PhysicShape &shape,
                              PhysicBodyType type, Vec3 &pos,
                              const Quaternion &quat) {
    JPH::ShapeRefC shape_ref = this->create_shape(shape);
    if (shape_ref.GetPtr() == nullptr) {
        SPDLOG_ERROR("Invalid shape type.");
        return;
    }
    JPH::BodyInterface &body_if = this->system.GetBodyInterface();
    JPH::EMotionType m_type;
    switch (type) {
        case PhysicBodyType::STATIC:
            m_type = JPH::EMotionType::Static;
            break;
        case PhysicBodyType::DYNAMIC:
            m_type = JPH::EMotionType::Dynamic;
            break;
        case PhysicBodyType::KINETIC:
            m_type = JPH::EMotionType::Kinematic;
            break;
    }

    JPH::BodyCreationSettings setting(shape_ref, to_jolt(pos), to_jolt(quat),
                                      m_type, Layers::MOVING);
    JPH::BodyID id =
        body_if.CreateAndAddBody(setting, JPH::EActivation::Activate);
    Handle handle = this->bodys.insert(id);
    body.handle = handle;
}
void JoltBackend::delete_body(PhysicBody &body) {
    JPH::BodyID *body_id = this->bodys.get_or_null(body.handle);
    EXPECT_NOT_NULL_RET(body_id);
    JPH::BodyInterface &body_if = this->system.GetBodyInterface();

    body_if.DestroyBody(*body_id);
    this->bodys.remove(body.handle);
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo,
                                 JPH::ColorArg inColor) {
    DebugDrawer::get_instance()->draw_line(from_jolt(inFrom), from_jolt(inTo),
                                           from_jolt(inColor));
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2,
                                     JPH::RVec3Arg inV3, JPH::ColorArg inColor,
                                     ECastShadow inCastShadow) {
    DebugDrawer::get_instance()->draw_triangle(
        from_jolt(inV1), from_jolt(inV2), from_jolt(inV3), from_jolt(inColor));
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(
    const Triangle *inTriangles, int inTriangleCount) {
    return Batch();
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(
    const Vertex *inVertices, int inVertexCount, const uint32_t *inIndices,
    int inIndexCount) {
    return Batch();
}

void JoltDebugRenderer::DrawGeometry(
    JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds,
    float inLODScaleSq, JPH::ColorArg inModelColor,
    const GeometryRef &inGeometry, ECullMode inCullMode,
    ECastShadow inCastShadow, EDrawMode inDrawMode) {}

void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition,
                                   const std::string_view &inString,
                                   JPH::ColorArg inColor, float inHeight) {}

JoltDebugRenderer::JoltDebugRenderer() { DebugRenderer::Initialize(); }

}  // namespace Seed