#pragma once

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <Windows.h>
#include <bcrypt.h>

#include <ext/core/defines.h>

using NtDelayExecution_t = NTSTATUS(WINAPI*)(BOOL Alertable, PLARGE_INTEGER DelayInterval);
using ZwSetTimerResolution_t = NTSTATUS(WINAPI*)(ULONG RequestedResolution, BOOLEAN Set, PULONG ActualResolution);

static const NtDelayExecution_t kNtDelayExecution =
    (NtDelayExecution_t)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtDelayExecution");
static const ZwSetTimerResolution_t kZwSetTimerResolution =
    (ZwSetTimerResolution_t)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "ZwSetTimerResolution");

#else
#include <thread>
#endif

namespace ext::thread_details {

// Improved sleep function which allows to sleep for less then 15 milliseconds
inline void sleep_for(long long milliseconds)
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    // On Windows, when you call Sleep(1), you're asking the system to sleep for 1 millisecond. 
    // Because of the timer resolution, the system cannot guarantee a sleep period smaller than its timer tick.
    // Therefore, Sleep(1) will often result in a sleep duration of one timer tick, which is about 15.6 milliseconds on most systems.
    static const auto once = []()
    {
        ULONG actualResolution;
        kZwSetTimerResolution(1, true, &actualResolution);
        return true;
    }();
    EXT_UNUSED(once);

    LARGE_INTEGER interval;
    interval.QuadPart = -1 * (milliseconds * 10000);
    // We will use NtDelayExecution to make 1 millisecond sleep exactly 1 millisecond sleep
    kNtDelayExecution(false, &interval);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

} // namespace ext::thread_details
