#ifndef _SEED_SHAPE_H_
#define _SEED_SHAPE_H_
#include "core/types.h"

namespace Seed{
    struct Rect{
        u32 x, y, w, h;
    };
    
    struct RectF{
        f32 x, y, w, h;
    };
}

#endif