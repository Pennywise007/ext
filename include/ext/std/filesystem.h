#pragma once

#include <fstream>
#include <filesystem>

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <libloaderapi.h>
#elif defined(__GNUC__) // linux
#include <unistd.h>
#endif // linux

#include <ext/core/defines.h>
#include <ext/error/dump_writer.h>

namespace std::filesystem {

[[nodiscard]] inline std::filesystem::path get_full_exe_path() noexcept
{
    constexpr auto kMaxPath = 4096;
    char exePath[kMaxPath] = {0};  
#if defined(_WIN32) || defined(__CYGWIN__)
    EXT_DUMP_IF(GetModuleFileNameA(/*AfxGetApp()->m_hInstance*/nullptr, exePath, kMaxPath - 1) == 0);
#elif __GNUC__ // windows
    const auto len = readlink("/proc/self/exe", exePath, kMaxPath - 1);
    if (len == -1) {
        EXT_DUMP_IF(true);
        return "";
    }
    exePath[len] = '\0';
#endif // linux
    return std::filesystem::path(exePath);
}

[[nodiscard]] inline std::filesystem::path get_exe_directory() noexcept
{
    return get_full_exe_path().remove_filename();
}

[[nodiscard]] inline std::filesystem::path get_exe_name() noexcept
{
    return get_full_exe_path().filename();
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
