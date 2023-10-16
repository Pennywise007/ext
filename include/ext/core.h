#pragma once

#include <locale>

#ifdef __AFX_H__
#include "core/check.h"
#include "thread/invoker.h"
#include "thread/tick.h"
#endif

#include "core/defines.h"
#include "core/dispatcher.h"
#include "core/singleton.h"
#include "error/dump_writer.h"
#include "thread/thread.h"
#include "trace/tracer.h"

namespace ext::core {

template <typename T>
struct numbers_formatter : std::numpunct<T>
{
    typename std::numpunct<T>::char_type do_thousands_sep() const override { return {'\0'}; }
    typename std::numpunct<T>::char_type do_decimal_point() const override { return {','}; }
};

// initialization for ext library(not necessary, but useful)
inline void Init()
{
    // for Russian output in concole
    setlocale(LC_ALL, "Russian");

    // install all languages support in stream 
    const std::locale baseLoc = std::locale("");
    // setup numeric formats for C++ streams, exclude 1,000.23 situations
    const auto localeWithFixedSeparators = std::locale(std::locale(baseLoc, new numbers_formatter<wchar_t>()), new numbers_formatter<char>());
    std::locale::global(localeWithFixedSeparators);

#if defined(_WIN32) || defined(__CYGWIN__) // windows
    EXT_DUMP_DECLARE_HANDLER();
#endif

#ifdef __AFX_H__
    if (afxCurrentAppName == NULL) // avoid double initialization
        EXT_EXPECT(AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) << L"Failed to initalize afx";
    ext::get_service<ext::invoke::MethodInvoker>().Init();
#endif

    // initializing some core services so that they are the last to be destroyed
    EXT_IGNORE_RESULT(ext::get_tracer());
    EXT_IGNORE_RESULT(ext::thread::manager());
    EXT_IGNORE_RESULT(ext::get_service<ext::events::Dispatcher>());
#ifdef __AFX_H__
    EXT_IGNORE_RESULT(ext::get_service<ext::tick::TickService>());
#endif
}

} // namespace ext
