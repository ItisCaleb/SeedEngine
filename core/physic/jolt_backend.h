#ifndef _SEED_JOLT_BACKEND_H_
#define _SEED_JOLT_BACKEND_H_

namespace Seed{
    class JoltBackend{
        static void trace(const char *inFMT, ...);

        public:
            JoltBackend();
    };
    
}

#endif