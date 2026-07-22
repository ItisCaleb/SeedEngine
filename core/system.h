#ifndef _SEED_SYSTEM_H_
#define _SEED_SYSTEM_H_

namespace Seed {
class SeedEngine;
class RenderEngine;
class ResourceEntries;
class ResourceLoader;
class GuiEngine;
class Input;
class DefaultStorage;
class DebugDrawer;
class ThreadPool;
class PhysicEngine;
class Profiler;
#ifdef SEED_XR
class XREngine;
#endif

namespace System {
extern SeedEngine *gEngine;
extern RenderEngine *gRenderEngine;
extern ResourceEntries *gResourceEntries;
extern ResourceLoader *gResourceLoader;
extern GuiEngine *gGuiEngine;
extern Input *gInput;
extern DefaultStorage *gDefaultStorage;
extern DebugDrawer *gDebugDrawer;
extern ThreadPool *gThreadPool;
extern PhysicEngine *gPhysicEngine;
extern Profiler *gProfiler;
#ifdef SEED_XR
extern XREngine *gXREngine;
#endif
};  // namespace System
}  // namespace Seed

#endif