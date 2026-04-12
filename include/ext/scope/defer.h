/*
Realization of the defer from GoLang
The code will be executed on leaving the current scope

auto file = std::open_file(...)
EXT_DEFER(file.close(); other_func());
*/

#pragma once

#include <ext/scope/on_exit.h>

/* Setup execution code on exit scope

Using:
EXT_DEFER(foundedPos = currentPos);
EXT_DEFER_F([&foundedPos, currentPos]() {
    foundedPos = currentPos;
});

Equal to:
ext::scope::ExitScope __on_scoped_exit0([&]() {
    code;
});
*/
#define EXT_DEFER(code) ::ext::scope::ExitScope EXT_PP_CAT(__on_scoped_exit, __COUNTER__)([&]() { code; });
#define EXT_DEFER_F(func) ::ext::scope::ExitScope EXT_PP_CAT(__on_scoped_exit, __COUNTER__)(func);
