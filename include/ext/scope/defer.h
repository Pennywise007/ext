/*
Realization of the defer from GoLang
The code will be executed on leaving the current scope

auto file = std::open_file(...)
EXT_DEFER({ file.close() });
*/

#pragma once

#include <ext/scope/on_exit.h>

/* Setup execution code on exit scope

Using:
* EXT_DEFER({ foundedPos = currentPos; });
* EXT_DEFER_F((&foundedPos, currentPos),
    {
        foundedPos = currentPos;
    });
*/
#define EXT_DEFER EXT_SCOPE_ON_EXIT
#define EXT_DEFER_F EXT_SCOPE_ON_EXIT_F
