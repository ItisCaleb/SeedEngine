#include "fiber.h"
#include "core/types.h"

namespace Seed {
void Fiber::fiber_entry(void *old_context){
    KContext *ctx = (KContext*)old_context;
    Fiber *fiber = (Fiber*)ctx->user_data;
    fiber->yield_ctx = ctx;
    fiber->entry_func(fiber);
    fiber->status = FiberStatus::DONE;
    _jump_context(old_context, nullptr);
    __builtin_unreachable();
}

Fiber::Fiber(void (*entry_func)(Fiber *)) {
    this->entry_func = [=](Fiber *fiber){
        entry_func(fiber);
    };
    u64 sp = (u64)this->stack + sizeof(this->stack);
    /* preserve some space */
    sp -= sizeof(KContext);
    KContext *context = (KContext*)sp;
    context->rbp = (void*)(sp + sizeof(KContext));
    context->rip = (void*)Fiber::fiber_entry;
    this->ctx = context;
}

void Fiber::resume(){
    if(this->status == FiberStatus::DONE){
        return;
    }
    this->status = FiberStatus::RUNNING;
    void *old_context = _jump_context(this->ctx, this);
    this->ctx = (KContext*)old_context;
}

void Fiber::yield(){
    if(this->status != FiberStatus::RUNNING || this->yield_ctx == nullptr){
        return;
    }
    this->status = FiberStatus::WAITING;
    void *old_context = _jump_context(this->yield_ctx, this);
    this->yield_ctx = (KContext*)old_context;
}


}  // namespace Seed