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
    private:
        RectF dimension;
        Window *binded_window;

    public:
        Viewport(Window *window, RectF dimension) : binded_window(window) {
            set_dimension(dimension);
        }
        Viewport(Window *window, f32 x, f32 y, f32 w, f32 h)
            : Viewport(window, RectF{x, y, w, h}) {}
        Viewport(Window *window) : Viewport(window, 0, 0, 1, 1) {}

        void set_dimension(RectF dim) {
            dimension.x = clampf(dim.x, 0, 1);
            dimension.y = clampf(dim.y, 0, 1);
            dimension.w = clampf(dim.w, 0, 1);
            dimension.h = clampf(dim.h, 0, 1);
        }

        void set_dimension(f32 x, f32 y, f32 w, f32 h) {
            set_dimension(RectF{x, y, w, h});
        }

        RectF get_dimension() { return dimension; }

        Rect get_actual_dimension() {
            if (!binded_window) {
                SPDLOG_ERROR("This viewport doesn't bind a window.");
                return {};
            }
            u32 actual_w = binded_window->get_width();
            u32 actual_h = binded_window->get_height();
            return Rect{.x = (u32)(actual_w * dimension.x),
                        .y = (u32)(actual_h * dimension.y),
                        .w = (u32)(actual_w * dimension.w),
                        .h = (u32)(actual_h * dimension.h)};
        }

        bool within_viewport(f32 x, f32 y) {
            return x >= dimension.x && x <= dimension.x + dimension.w &&
                   y >= dimension.y && y <= dimension.y + dimension.h;
        }

        bool within_viewport(Vec2 pos) { return within_viewport(pos.x, pos.y); }

        Vec2 to_viewport_coord(f32 x, f32 y) {
            return Vec2{
                (x - dimension.x) / dimension.w,
                (y - dimension.y) / dimension.h,
            };
        }

        Vec2 to_viewport_coord(Vec2 pos) {
            return to_viewport_coord(pos.x, pos.y);
        }
};
}  // namespace Seed

#endif