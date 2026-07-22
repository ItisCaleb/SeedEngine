#ifndef _SEED_RENDERER_H_
#define _SEED_RENDERER_H_
#include "core/rendering/rhi/render_command.h"

namespace Seed {

class Window;

namespace GlobalBinding {
constexpr u32 Visible = 0;
constexpr u32 Transform = 1;
constexpr u32 Terrain = 2;
constexpr u32 Bones = 3;
constexpr u32 Camera = 8;
constexpr u32 Lights = 9;
constexpr u32 CSM = 10;
constexpr u32 Projection = 11;

};  // namespace GlobalBinding

struct FrameGlobal {
        SSBOHandle visible;
        SSBOHandle transform;
        SSBOHandle terrain;
        SSBOHandle bones;
        ConstantHandle camera;
        ConstantHandle lights;
        ConstantHandle csm;
        ConstantHandle projection;
        void init();
        void bind(RenderStateDataBuilder &builder);
};

class Renderer {
        virtual void _process(RenderCommandDispatcher &dp) = 0;

    public:
        virtual void init(Window *window) = 0;
        virtual void preprocess() = 0;
        void process(u8 layer) {
            RenderCommandDispatcher dp;
            dp.set_layer(layer);
            _process(dp);
        }
        virtual void cleanup() = 0;
};

}  // namespace Seed

#endif
