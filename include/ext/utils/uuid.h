#pragma once

#include <string>

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <atlbase.h>
#include <combaseapi.h>
#elif defined(__GNUC__) // linux
#include <uuid/uuid.h>
#endif

#include <ext/core/defines.h>
#include <ext/std/string.h>

namespace ext {

struct uuid
{
    explicit uuid()
    {
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        EXT_EXPECT(SUCCEEDED(CoCreateGuid(&m_id))) << "Failed to create GUID";
#elif defined(__GNUC__) // linux
        uuid_generate(m_id);
#endif
    }

    EXT_NODISCARD bool operator==(const uuid& other) const EXT_NOEXCEPT { return memcmp(&m_id, &other.m_id, sizeof(m_id)) == 0; }
    EXT_NODISCARD bool operator<(const uuid& other) const EXT_NOEXCEPT { return memcmp(&m_id, &other.m_id, sizeof(m_id)) < 0; }

    EXT_NODISCARD std::string ToString() const
    {
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        return std::narrow(CComBSTR(m_id));
#elif defined(__GNUC__) // linux
        std::string uuidStr(50, '\0');
        uuid_unparse(m_id, uuidStr.data());
        uuidStr.resize(uuidStr.find_last_not_of('\0') + 1);
        return uuidStr;
#endif
    }

#if defined(_WIN32) || defined(__CYGWIN__) // windows
    GUID m_id;
#elif defined(__GNUC__) // linux
    uuid_t m_id;
#endif
};

} // namespace ext