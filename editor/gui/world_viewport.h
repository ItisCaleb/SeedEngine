#ifndef _SEED_WORLD_VIEWPORT_H_
#define _SEED_WORLD_VIEWPORT_H_

#include <RmlUi/Core/Event.h>

#include "core/math/vec3.h"
#include "core/types.h"
#include "editor/world/world_renderer.h"

namespace Seed {

struct PickResult {
        u16 data[4];
};

class WorldViewport {
    private:
        WorldRenderer *renderer = nullptr;
        i32 pick_x = 0;
        i32 pick_y = 0;
        Vec3 focus_target;
        bool focus_valid = false;
        f32 scroll_delta = 0.0f;

    public:
        void init();
        void reset();

        bool is_hovered() const;
        bool event_to_pixel(Rml::Event &event, i32 &image_x,
                            i32 &image_y) const;
        bool pick_at_pixel(i32 image_x, i32 image_y, PickResult &result) const;

        void set_camera_focus(const Vec3 &target);
        void clear_camera_focus();
        bool get_camera_focus(Vec3 &target) const;

        void add_scroll(f32 delta);
        f32 consume_scroll();
};

}  // namespace Seed

#endif
