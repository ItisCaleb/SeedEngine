#ifndef _SEED_INPUT_HANDLER_H_
#define _SEED_INPUT_HANDLER_H_
#include "core/window.h"
namespace Seed {
class SeedEngine;
class InputHandler {
        friend SeedEngine;

    private:
        void init(Window *window);

    public:
        InputHandler() = default;
        ~InputHandler() = default;
};

}  // namespace Seed

#endif