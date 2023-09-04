#pragma once

#ifdef __GNUG__
#include <cxxabi.h>
#endif

#include <ext/core/defines.h>

namespace ext {

template <typename Type>
EXT_NODISCARD const char* type_name()
{
#ifdef __GNUG__
    return abi::__cxa_demangle(typeid(Type).name(), NULL, NULL, NULL);

#else
    return typeid(Type).name();
#endif
}

} // namespace ext