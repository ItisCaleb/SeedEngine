#include "jolt_backend.h"
#include <thread>
#include <spdlog/spdlog.h>
#include "core/debug/debug_drawer.h"

namespace Seed {

void JoltBackend::trace(const char *inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    SPDLOG_DEBUG("{}", buffer);
}

inline static Vec3 from_jolt_vec3(JPH::Vec3 from) {
    return Vec3{from.GetX(), from.GetY(), from.GetZ()};
}

inline static Color from_jolt_color(JPH::Color from) {
    return Color{from.r, from.g, from.b, from.a};
}

JoltBackend::JoltBackend() {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = trace;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::DebugRenderer::sInstance = new JoltDebugRenderer;
    JPH::RegisterTypes();
    const uint cMaxBodies = 65536;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 65536;
    const uint cMaxContactConstraints = 10240;
    this->job_system = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1);
    this->temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                cMaxContactConstraints, broad_phase_layer_interface,
                object_vs_broadphase_layer_filter,
                object_vs_object_layer_filter);

    JPH::BodyInterface &body_interface = system.GetBodyInterface();
}

void JoltBackend::process() {
    system.Update(1 / 60, 1, temp_allocator, job_system);
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo,
                                 JPH::ColorArg inColor) {
    DebugDrawer::get_instance()->draw_line(
        from_jolt_vec3(inFrom), from_jolt_vec3(inTo), from_jolt_color(inColor));
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2,
                                     JPH::RVec3Arg inV3, JPH::ColorArg inColor,
                                     ECastShadow inCastShadow) {
    DebugDrawer::get_instance()->draw_triangle(
        from_jolt_vec3(inV1), from_jolt_vec3(inV2), from_jolt_vec3(inV3),
        from_jolt_color(inColor));
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