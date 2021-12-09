#pragma once

#include <fstream>
#include <filesystem>
#include <libloaderapi.h>

#include <ext/core/defines.h>
#include <ext/error/dump_writer.h>

namespace std::filesystem {

EXT_NODISCARD inline std::filesystem::path get_exe_directory() EXT_NOEXCEPT
{
    const static auto currentDirectory = []() -> std::filesystem::path
    {
        wchar_t exePath[_MAX_PATH * 10] = {0};
        EXT_DUMP_IF(GetModuleFileName(/*AfxGetApp()->m_hInstance*/nullptr, exePath, _MAX_PATH * 10) == 0);
        return std::filesystem::path(exePath).remove_filename();
    }();
    return currentDirectory;
}

EXT_NODISCARD inline std::filesystem::path get_exe_name() EXT_NOEXCEPT
{
    const static auto exeName = []() -> std::filesystem::path
    {
        wchar_t exePath[_MAX_PATH * 10] = {0};
        EXT_DUMP_IF(GetModuleFileName(/*AfxGetApp()->m_hInstance*/nullptr, exePath, _MAX_PATH * 10) == 0);

        return std::filesystem::path(exePath).filename();
    }();
    return exeName;
}

inline bool create_file(const std::filesystem::path& path) EXT_THROWS(std::filesystem::filesystem_error)
{
    if (!std::filesystem::create_directories(path))
        return false;

    std::wofstream ofs(path.c_str());
    ofs << L'\0';
    ofs.close();
    return true;
}

} // namespace std::filesystem
