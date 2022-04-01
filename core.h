#pragma once

#include "error/dump_writer.h"
#include "trace/tracer.h"
#include "thread/invoker.h"

namespace ext::core {

// initialization for ext library(not neccessary, but usefull)
inline void Init()
{
    std::locale::global(std::locale(""));
    EXT_DUMP_DECLARE_HANDLER();
    ext::get_service<ext::invoke::MethodInvoker>().Init();
}

} // namespace ext
