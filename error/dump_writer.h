#pragma once

#include <atomic>
#include <Dbghelp.h>
#include <shlwapi.h>
#include <Windows.h>

#include <ext/core/defines.h>
#include <ext/scope/on_exit.h>
#include <ext/trace/itracer.h>
#include <ext/utils/call_once.h>

/*
* Example of creating dump
void main()
{
    EXT_DUMP_DECLARE_HANDLER();

    EXT_DUMP_CREATE() OR throw std::exception();
}
*/

/* Declare handler for unhandled exceptions and create dumps */
#define EXT_DUMP_DECLARE_HANDLER()                                          \
    if (!::ext::dump::g_exceptionHandlerAdded)                              \
    {                                                                       \
        ::ext::dump::g_exceptionHandlerAdded = true;                        \
        ::AddVectoredExceptionHandler(1, ::ext::dump::unhandled_handler);   \
    }

/* Create dump code, it is better to call EXT_DUMP_DECLARE_HANDLER in main to catch unhandled exceptions */
#define EXT_DUMP_CREATE() ext::dump::create_dump();

// If the debugger is connected - break, otherwise - create a process dump
#define DEBUG_BREAK_OR_CREATE_DUMP                                          \
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
    if (const bool __result = (bool_expression); !__result) {}      \
    else                                                            \
        for (bool __firstEnter = true;; __firstEnter = false)       \
            if (!__firstEnter)                                      \
            {                                                       \
                CALL_ONCE(DEBUG_BREAK_OR_CREATE_DUMP)               \
                break;                                              \
            }                                                       \
            else                                                    \
                EXT_TRACE_ERR() << "DUMP at " << __FILE__ << "(" << __LINE__ << ") expr: \'" << #bool_expression << "\' "

namespace ext::dump {

constexpr unsigned long long g_exceptionWriteDumpAndContinue = 0x42849fc0;

static std::atomic_bool g_exceptionHandlerAdded = false;

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

    EXT_TRACE() << "Dump file: " << name;

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

inline void create_dump(const char *msg = nullptr) EXT_NOEXCEPT
{
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