#ifndef _SEED_POPUP_H_
#define _SEED_POPUP_H_

class Popup {
    public:
        bool should_close = false;
        virtual void draw() = 0;
        virtual ~Popup() {}
};

#endif
