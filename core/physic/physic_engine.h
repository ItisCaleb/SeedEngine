#ifndef _SEED_PHYSIC_ENGINE_H_
#define _SEED_PHYSIC_ENGINE_H_
namespace Seed {
    
class PhysicEngine {
    private:
        inline static PhysicEngine* instance = nullptr;
        static void trace(const char *inFMT, ...);
    public:
        static PhysicEngine* get_instance(){
            return instance;
        }
        
        PhysicEngine();

};
}  // namespace Seed

#endif