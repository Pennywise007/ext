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
* EXT_DEFER(foundedPos = currentPos);
* EXT_DEFER_F((&foundedPos, currentPos),
        foundedPos = currentPos;
    );
*/
#define EXT_DEFER(code) EXT_SCOPE_ON_EXIT({ code; })
#define EXT_DEFER_F(capture, code) EXT_SCOPE_ON_EXIT_F(capture, { code })
