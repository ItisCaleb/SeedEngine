#include "dedicated_thread.h"
#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#pragma comment(lib, "Kernel32.lib")

#endif

namespace Seed {
thread_local bool DedicatedThread::is_coroutine = false;

void DedicatedThread::set_thread_name(const std::string &name) {
#ifdef _WIN32
    std::wstring wname(name.begin(), name.end());
    HANDLE hThread = th.native_handle();
    HMODULE hKernel32 = GetModuleHandle("Kernel32.dll");
    if (hKernel32) {
        typedef HRESULT(WINAPI * pfnSetThreadDescription)(HANDLE, PCWSTR);
        pfnSetThreadDescription SetThreadDescriptionFunc =
            (pfnSetThreadDescription)GetProcAddress(hKernel32,
                                                    "SetThreadDescription");

        if (SetThreadDescriptionFunc) {
            SetThreadDescriptionFunc(hThread, wname.c_str());
        }
    }
#endif
}

void DedicatedThread::thread_func(Entrypoint entry, bool is_coroutine) {
    DedicatedThread::is_coroutine = is_coroutine;
    while (true) {
        entry();
    }
}

DedicatedThread::DedicatedThread(const std::string &name, Entrypoint entry,
                                 bool is_coroutine)
    : th(thread_func, entry, is_coroutine) {
    set_thread_name(name);
}
}  // namespace Seed