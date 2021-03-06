#pragma once

#include <locale>

#ifdef __AFX_H__
#include "core/check.h"
#include "thread/invoker.h"
#include "thread/tick.h"
#endif

#include "core/dispatcher.h"
#include "core/singleton.h"
#include "error/dump_writer.h"
#include "trace/tracer.h"

namespace ext::core {

template <typename T>
struct numbers_formatter : std::numpunct<T>
{
    char_type do_thousands_sep() const override { return char_type('\0'); }
    char_type do_decimal_point() const override { return char_type(','); }
};

// initialization for ext library(not neccessary, but usefull)
inline void Init()
{
    // for Russian output in concole
    setlocale(LC_ALL, "Russian");

    // install all languages support in stream 
    const std::locale baseLoc = std::locale("");
    // setup numeric formats for C++ streams, exclude 1,000.23 situations
    const auto localeWithFixedSeparators = std::locale(std::locale(baseLoc, new numbers_formatter<wchar_t>()), new numbers_formatter<char>());
    std::locale::global(localeWithFixedSeparators);

    EXT_DUMP_DECLARE_HANDLER();

#ifdef __AFX_H__
    if (afxCurrentAppName == NULL) // avoid double initialization
        EXT_EXPECT(AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) << L"Failed to initalize afx";
    ext::get_service<ext::invoke::MethodInvoker>().Init();
#endif

    // initializing some core services so that they are the last to be destroyed
    ext::get_service<ext::events::Dispatcher>();
#ifdef __AFX_H__
    ext::get_service<ext::tick::TickService>();
#endif
}

} // namespace ext
