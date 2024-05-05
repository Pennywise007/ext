#ifdef __AFX_H__

/*
Allows to execute code in main UI thread.

Example:

ext::InvokeMethod([&]() {
        MessageBox(L"Error", L"", MB_OK);
        EndDialog(501);
    });

ext::InvokeMethodAsync([&]() {
        MessageBox(L"Error", L"", MB_OK);
        EndDialog(501);
    });
*/

#pragma once

#include <afxwin.h>
#include <functional>
#include <set>
#include <tlhelp32.h>

#include <ext/core/check.h>
#include <ext/core/defines.h>
#include <ext/core/singleton.h>

#include <ext/types/utils.h>

namespace ext {
namespace invoke {

/* Realization of synchronization class with program UI thread, creation must be from main UI thread, @see Init
   Can be used to synchronize call with GUI */
class MethodInvoker : private CWnd
{
    friend ext::Singleton<MethodInvoker>;

    MethodInvoker();
    ~MethodInvoker();

// passing a function for execution in the main thread of the application
public:
    // Initialization function, should be called from main thread
    void Init();

    typedef std::function<void()> CallFunction;
    // synch execution of the function in main thread
    void CallSync(CallFunction&& func) const EXT_THROWS();
    // async execution of the function in main thread
    void CallAsync(CallFunction&& func) EXT_THROWS();
    // checking that we are in the main thread
    [[nodiscard]] static bool IsMainThread();

private:
    // create hidden window, must be called from main thread
    void CreateMainThreadWindow();
    // function to get the id of the main thread
    static DWORD GetMainThreadId();

private:
    struct CallFuncInfo
    {
        CallFuncInfo(bool syncCall, CallFunction&& func) : callFunc(std::move(func)), synchroniousCall(syncCall) {}

        CallFunction callFunc;
        const bool synchroniousCall;                // if true then we need to return answer and handled exception, false - destroy object
        std::exception_ptr exception = nullptr;     // an exception that may have been thrown during the execution of a function
    };
    // A message to the main thread window, about the need to call function, lParam = CallFunction
    static constexpr UINT kCallFunctionMessage = WM_USER + 1;

    // id of the thread in which the message window is running
    DWORD m_windowThreadId = NULL;
};

inline MethodInvoker::MethodInvoker()
    : m_windowThreadId(::GetCurrentThreadId())
{
    CreateMainThreadWindow();
}

inline MethodInvoker::~MethodInvoker()
{
    if (::IsWindow(m_hWnd))
        ::DestroyWindow(m_hWnd);
}

inline void MethodInvoker::Init()
{
    if (m_windowThreadId != GetMainThreadId())
    {
        if (::IsWindow(m_hWnd))
            ::DestroyWindow(m_hWnd);

        CreateMainThreadWindow();
    }
}

inline void MethodInvoker::CallSync(CallFunction&& func) const EXT_THROWS()
{
    if (IsMainThread())
        func();         // main thread - execute immediately
    else
    {
        // if called from an auxiliary thread - send a message with the required function to the main thread
        EXT_EXPECT(::IsWindow(m_hWnd)) << EXT_TRACE_FUNCTION;
        auto callFunc = std::make_shared<CallFuncInfo>(true, std::move(func));

        // send a message to the window in order to process the message in the main UI thread
        EXT_DUMP_IF(CWnd::SendMessage(kCallFunctionMessage, 0, reinterpret_cast<LPARAM>(callFunc.get())) != 0);

        // if there was an unhandled exception during execution, throw it here
        if (callFunc->exception)
            std::rethrow_exception(callFunc->exception);
    }
}

inline void MethodInvoker::CallAsync(CallFunction&& func) EXT_THROWS()
{
    // send a message with the required function to the main thread
    EXT_EXPECT(::IsWindow(m_hWnd)) << EXT_TRACE_FUNCTION;

    // if called from an auxiliary thread - send a message with the required function to the main thread
    auto callFunc = std::make_unique<CallFuncInfo>(false, std::move(func));
    EXT_DUMP_IF(CWnd::PostMessage(kCallFunctionMessage, 0, reinterpret_cast<LPARAM>(callFunc.release())) != TRUE);
}

[[nodiscard]] inline bool MethodInvoker::IsMainThread()
{
    return get_singleton<MethodInvoker>().m_windowThreadId == ::GetCurrentThreadId();
}

inline void MethodInvoker::CreateMainThreadWindow()
{
    EXT_DUMP_IF(m_windowThreadId != ::GetCurrentThreadId()) << "Creation of MethodInvoker must be called from main thread!";

    HINSTANCE instance = AfxGetInstanceHandle();
    const char* className(ext::type_name<decltype(*this)>());

    // Registering internal window
    WNDCLASSEXA wndClass;
    if (!::GetClassInfoExA(instance, className, &wndClass))
    {
        memset(&wndClass, 0, sizeof(WNDCLASSEXA));
        wndClass.cbSize = sizeof(WNDCLASSEXA);
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = [](HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            switch (Message)
            {
            case kCallFunctionMessage:
            {
                CallFuncInfo* callFuncInfo(reinterpret_cast<CallFuncInfo*>(lparam));

                try
                {
                    callFuncInfo->callFunc();
                    if (!callFuncInfo->synchroniousCall)
                        delete callFuncInfo;
                }
                catch (...)
                {
                    if (callFuncInfo->synchroniousCall)
                        // return exception to caller
                        callFuncInfo->exception = std::current_exception();
                    else
                        delete callFuncInfo;
                }

                return 0;
            }
            default:
                return ::DefWindowProc(hwnd, Message, wparam, lparam);
            }
        };

        // TODO CHECK IF NEEDED
        wndClass.hInstance = instance;
        wndClass.lpszClassName = className;

        EXT_EXPECT(RegisterClassExA(&wndClass)) << EXT_TRACE_FUNCTION << "Can`t register class";
    }

    EXT_DUMP_IF(CWnd::CreateEx(0, CString(className), L"", 0, 0, 0, 0, 0, NULL, nullptr, nullptr) == FALSE)
        << EXT_TRACE_FUNCTION << "Can`t create synchronization window";
}

inline DWORD MethodInvoker::GetMainThreadId()
{
    DWORD result = 0;

    const std::shared_ptr<void> hThreadSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);
    if (hThreadSnapshot.get() != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 tEntry;
        tEntry.dwSize = sizeof(THREADENTRY32);
        const DWORD currentPID = GetCurrentProcessId();
        for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
             !result && success && GetLastError() != ERROR_NO_MORE_FILES;
             success = Thread32Next(hThreadSnapshot.get(), &tEntry))
        {
            if (tEntry.th32OwnerProcessID == currentPID)
                result = tEntry.th32ThreadID;
        }
    }
    else
        EXT_DUMP_IF(true) << "Failed";

    return result;
}

} // namespace invoke

inline void InvokeMethod(::ext::invoke::MethodInvoker::CallFunction&& function)
{
    get_singleton<::ext::invoke::MethodInvoker>().CallSync(std::forward<::ext::invoke::MethodInvoker::CallFunction>(function));
}

inline void InvokeMethodAsync(std::function<void()>&& function)
{
    get_singleton<::ext::invoke::MethodInvoker>().CallAsync(std::forward<::ext::invoke::MethodInvoker::CallFunction>(function));
}

} // namespace ext

#endif // __AFX_H__
