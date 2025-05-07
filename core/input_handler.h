#ifndef _SEED_INPUT_HANDLER_H_
#define _SEED_INPUT_HANDLER_H_
namespace Seed {
class SeedEngine;
class InputHandler {
        friend SeedEngine;

    private:
        template <class T>
        void init(T *window);

    public:
        InputHandler() = default;
        ~InputHandler() = default;
};

}  // namespace Seed

#endif