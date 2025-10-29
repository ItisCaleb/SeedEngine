#ifndef _SEED_VIEWPORT_H_
#define _SEED_VIEWPORT_H_
#include "core/types.h"
#include "core/window.h"
#include "core/collision/shape.h"
#include "core/math/utils.h"
#include "core/math/vec2.h"
#include <spdlog/spdlog.h>

namespace Seed {
class Viewport {
    protected:
        RectF dimension;
        Vec2 size;

    public:
        Viewport(RectF dimension, Vec2 size) : size(size) {
            set_dimension(dimension);
        }

        Viewport(Vec2 size) : size(size) { set_dimension(RectF{0, 0, 1, 1}); }

        void set_dimension(RectF dim, bool flip_y = false);

        void set_dimension(f32 x, f32 y, f32 w, f32 h, bool flip_y = false);

        RectF get_dimension();

        /* The engine viewport's origin is top left */
        /* For API like OpenGL, we need to reverse y axis */
        virtual RectF get_actual_dimension(bool flip_y = false);

        bool within_viewport(f32 x, f32 y);

        bool within_viewport(Vec2 pos);

        Vec2 to_viewport_coord(f32 x, f32 y);

        Vec2 to_viewport_coord(Vec2 pos);
};

class WindowViewport : public Viewport {
    private:
        Window *window;

    public:
        WindowViewport(Window *window, RectF dimension)
            : Viewport(dimension, Vec2{0, 0}), window(window) {}
        WindowViewport(Window *window, f32 x, f32 y, f32 w, f32 h)
            : WindowViewport(window, RectF{x, y, w, h}) {}
        WindowViewport(Window *window) : WindowViewport(window, 0, 0, 1, 1) {}
        RectF get_actual_dimension(bool flip = false) override;
};
}  // namespace Seed

#endif