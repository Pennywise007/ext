#pragma once

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace ext {

// Allows to get a type name with valid format not depending on a platform
template <typename Type>
[[nodiscard]] const char* type_name()
{
#ifdef __GNUG__
    return abi::__cxa_demangle(typeid(Type).name(), NULL, NULL, NULL);

#else
    return typeid(Type).name();
#endif
}

} // namespace ext