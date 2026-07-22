#ifndef _SEED_INPUT_HANDLER_H_
#define _SEED_INPUT_HANDLER_H_
namespace Seed {
class SeedEngine;
class Window;
class InputHandler {
        friend SeedEngine;
        Window *window;

    private:
        void init(Window *window);

    public:
        InputHandler() = default;
        ~InputHandler() = default;
        void update();
};

}  // namespace Seed

#endif
