#pragma once

#include <string>
#include <Windows.h>

#include <ext/core/defines.h>
#include <ext/core/check.h>

namespace ext::registry {

struct Key
{
    Key(const wchar_t* registryPath, HKEY rootKey = HKEY_LOCAL_MACHINE, REGSAM accessMask = KEY_READ /*| KEY_WOW64_64KEY*/) SSH_THROWS(std::exception)
    {
        SSH_CHECK(::RegOpenKeyExW(rootKey, registryPath, 0, accessMask, &m_key) == ERROR_SUCCESS);
    }
    ~Key()
    {
        SSH_DUMP_IF(::RegCloseKey(m_key) != ERROR_SUCCESS) << SSH_TRACE_FUNCTION "Failed to close key";
    }

    SSH_NODISCARD bool GetRegistryValue(const wchar_t* valueName, DWORD& value) const SSH_NOEXCEPT
    {
        DWORD dwBufferSize(sizeof(DWORD));
        return ::RegQueryValueExW(m_key, valueName, 0, NULL, reinterpret_cast<LPBYTE>(&value), &dwBufferSize) == ERROR_SUCCESS;
    }

    SSH_NODISCARD bool GetRegistryValue(const wchar_t* valueName, bool& value) const SSH_NOEXCEPT
    {
        DWORD valueDword;
        if (GetRegistryValue(valueName, valueDword))
        {
            value = valueDword != 0;
            return true;
        }
        return false;
    }

    SSH_NODISCARD bool GetRegistryValue(const wchar_t* valueName, std::wstring& value) const SSH_NOEXCEPT
    {
        WCHAR szBuffer[MAX_PATH * 2];
        DWORD dwBufferSize = sizeof(szBuffer);
        if (RegQueryValueExW(m_key, valueName, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS)
        {
            value = szBuffer;
            return true;
        }
        return false;
    }

private:
    HKEY m_key;
};

} // namespace ext::registry