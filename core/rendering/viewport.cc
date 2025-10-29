#include "viewport.h"

namespace Seed {
void Viewport::set_dimension(RectF dim, bool flip_y) {
    dimension.x = clampf(dim.x, 0, 1);
    dimension.w = clampf(dim.w, 0, 1);
    dimension.h = clampf(dim.h, 0, 1);
    dimension.y =
        flip_y ? 1 - clampf(dim.y, 0, 1) - dimension.h : clampf(dim.y, 0, 1);
}

void Viewport::set_dimension(f32 x, f32 y, f32 w, f32 h, bool flip_y) {
    set_dimension(RectF{x, y, w, h}, flip_y);
}

RectF Viewport::get_dimension() { return dimension; }

RectF Viewport::get_actual_dimension(bool flip_y) {
    f32 y = flip_y ? size.y - (size.y * dimension.y) - (size.y * dimension.h)
                      : (size.y * dimension.y);
    return RectF{.x = (size.x * dimension.x),
                 .y = y,
                 .w = (size.x * dimension.w),
                 .h = (size.y * dimension.h)};
}

bool Viewport::within_viewport(f32 x, f32 y) {
    return x >= dimension.x && x <= dimension.x + dimension.w &&
           y >= dimension.y && y <= dimension.y + dimension.h;
}

bool Viewport::within_viewport(Vec2 pos) {
    return within_viewport(pos.x, pos.y);
}

Vec2 Viewport::to_viewport_coord(f32 x, f32 y) {
    return Vec2{
        (x - dimension.x) / dimension.w,
        (y - dimension.y) / dimension.h,
    };
}

Vec2 Viewport::to_viewport_coord(Vec2 pos) {
    return to_viewport_coord(pos.x, pos.y);
}

RectF WindowViewport::get_actual_dimension(bool flip_y) {
    u32 actual_w = window->get_width();
    u32 actual_h = window->get_height();
    f32 y = flip_y ? (f32)actual_w - (actual_w * dimension.y) -
                            (actual_w * dimension.h)
                      : (actual_w * dimension.y);
    return RectF{.x = (actual_w * dimension.x),
                 .y = (actual_h * dimension.y),
                 .w = (actual_w * dimension.w),
                 .h = (actual_h * dimension.h)};
}
}  // namespace Seed