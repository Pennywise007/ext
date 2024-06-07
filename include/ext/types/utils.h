#pragma once

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace ext {

// Allows to get a type name with valid format not depending on a platform
template <typename Type>
[[nodiscard]] const char* type_name()
{
    const char* typeName = typeid(Type).name();
#ifdef __GNUG__
    return abi::__cxa_demangle(typeName, NULL, NULL, NULL);
#else
    const auto demangle = [](const char* typeName) -> const char* {
        constexpr const char* prefixes[] = {"struct ", "class ", "enum "};
        for (const auto& prefix : prefixes) {
            const size_t prefixLen = strlen(prefix);
            if (strncmp(typeName, prefix, prefixLen) == 0) {
                return typeName + prefixLen;
            }
        }
        return typeName;
    };
    return demangle(typeName);
#endif
}

} // namespace ext