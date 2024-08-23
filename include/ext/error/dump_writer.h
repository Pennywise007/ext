#pragma once

#include <atomic>

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <Windows.h>
#include <debugapi.h>
#include <Dbghelp.h>
#include <shlwapi.h>
#elif __GNUC__ // linux
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <ext/core/tracer.h>
#include <ext/scope/on_exit.h>
#include <ext/utils/call_once.h>

/*
* Example of creating dump
void main()
{
    EXT_DUMP_DECLARE_HANDLER();

    EXT_DUMP_CREATE() OR throw std::exception();
}
*/

#if defined(_WIN32) || defined(__CYGWIN__) // windows
/* Declare handler for unhandled exceptions and create dumps */
#define EXT_DUMP_DECLARE_HANDLER()                                          \
    if (!::ext::dump::g_exceptionHandlerAdded)                              \
    {                                                                       \
        ::ext::dump::g_exceptionHandlerAdded = true;                        \
        ::AddVectoredExceptionHandler(1, ::ext::dump::unhandled_handler);   \
    }

/* Create dump code, it is better to call EXT_DUMP_DECLARE_HANDLER in main to catch unhandled exceptions */
#define EXT_DUMP_CREATE() ext::dump::create_dump();

#elif defined(__GNUC__) // linux

#define EXT_DUMP_CREATE() EXT_TRACE_ERR() << "Dump creation called, no dump file in linux";

[[nodiscard]] inline bool IsDebuggerPresent() {
    char buf[4096];

    const int status_fd = open("/proc/self/status", O_RDONLY);
    if (status_fd == -1)
        return false;

    const ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);
    close(status_fd);

    if (num_read <= 0)
        return false;

    buf[num_read] = '\0';
    constexpr char tracerPidString[] = "TracerPid:";
    const auto tracer_pid_ptr = strstr(buf, tracerPidString);
    if (!tracer_pid_ptr)
        return false;

    for (const char* characterPtr = tracer_pid_ptr + sizeof(tracerPidString) - 1; characterPtr <= buf + num_read; ++characterPtr)
    {
        if (isspace(*characterPtr))
            continue;
        else
            return isdigit(*characterPtr) != 0 && *characterPtr != '0';
    }

    return false;
}

#define DebugBreak() raise(SIGTRAP)

#endif // linux

// If the debugger is connected - break, otherwise - create a process dump
#define DEBUG_BREAK_OR_CREATE_DUMP()                                        \
    {                                                                       \
        if (IsDebuggerPresent())                                            \
            DebugBreak();                                                   \
        else                                                                \
            EXT_DUMP_CREATE();                                              \
    }

/*
Checks boolean expression, if true:
* generate breakpoint if debugger present, othervise - create dump
* show error in log*/
#define EXT_DUMP_IF(bool_expression)                                \
    if (const bool __result = (bool_expression); !__result)         \
    {}                                                              \
    else                                                            \
        for (bool __firstEnter = true;; __firstEnter = false)       \
            if (!__firstEnter)                                      \
            {                                                       \
                CALL_ONCE(DEBUG_BREAK_OR_CREATE_DUMP());            \
                break;                                              \
            }                                                       \
            else                                                    \
                EXT_TRACE_ERR() << "DUMP at " << __FILE__ << "(" << __LINE__ << ") expr: \'" << #bool_expression << "\' "

#if defined(_WIN32) || defined(__CYGWIN__) // windows

namespace ext::dump {

constexpr unsigned long long g_exceptionWriteDumpAndContinue = 0x42849fc0;

static std::atomic_bool g_dumpGenerationDisabled = true;
static std::atomic_bool g_exceptionHandlerAdded = false;

// Helper to disable dump generation, might be usefull in negative test cases
struct ScopeDumpDisabler
{
    ScopeDumpDisabler() noexcept { g_dumpGenerationDisabled = true; }
    ~ScopeDumpDisabler() noexcept { g_dumpGenerationDisabled = false; }
};

inline void make_minidump(EXCEPTION_POINTERS* e)
{
    EXT_TRACE_DBG() << "Dump creation started";

    auto hDbgHelp = ::LoadLibraryA("dbghelp");
    if (hDbgHelp == nullptr)
        return;
    ext::scope::FreeObject freeLibrary(hDbgHelp, &::FreeLibrary);

    auto writeDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (writeDump == nullptr)
        return;

    char name[MAX_PATH];
    {
        GetModuleFileNameA(GetModuleHandleA(0), name, MAX_PATH);
        PathRemoveExtensionA(name);

        SYSTEMTIME t;
        GetLocalTime(&t);
        wsprintfA(name + strlen(name), "_%4d.%02d.%02d_%02d.%02d.%02d.dmp", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    EXT_TRACE_DBG() << "Dump file: " << name;

    auto hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    ext::scope::FreeObject freeHandle(hFile, &CloseHandle);

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = e;
    exceptionInfo.ClientPointers = FALSE;

    writeDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
              MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory), // MiniDumpNormal
              &exceptionInfo, nullptr, nullptr);

    EXT_TRACE_DBG() << "Dump creation finished";
}

inline LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e)
{
    if (e && e->ExceptionRecord && e->ExceptionRecord->ExceptionCode == g_exceptionWriteDumpAndContinue)
        make_minidump(e);
    return EXCEPTION_CONTINUE_SEARCH;
}

inline void create_dump(const char *msg = nullptr) noexcept
{
    if (g_dumpGenerationDisabled)
        return;

    EXT_DUMP_DECLARE_HANDLER();
    __try
    {
        constexpr unsigned long argumentsCount = 1;
        constexpr unsigned long flags = 0;
        const ULONG_PTR arguments[argumentsCount] = { reinterpret_cast<ULONG_PTR>(msg) };
        ::RaiseException(g_exceptionWriteDumpAndContinue, flags, argumentsCount, arguments);
    }
    __except (-1)
    {}
}
} // namespace ext::dump

#endif