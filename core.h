#pragma once

#include <afx.h>
#include <locale>

#include "core/check.h"
#include "core/singleton.h"
#include "error/dump_writer.h"
#include "trace/tracer.h"
#include "thread/invoker.h"

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

    EXPECT_TRUE(AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) << L"Failed to initalize afx";
    ext::get_service<ext::invoke::MethodInvoker>().Init();
}

} // namespace ext
