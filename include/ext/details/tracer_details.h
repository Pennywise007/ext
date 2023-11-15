#pragma once

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <windows.h>
#include <debugapi.h>
#endif

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <ext/core/defines.h>
#include <ext/core/mpl.h>
#include <ext/core/noncopyable.h>

#include <ext/std/filesystem.h>

#include <ext/core/tracer.h>

namespace ext::tracer::details {

struct FileTracer : ::ext::ITracer, ::ext::NonCopyable
{
    FileTracer(std::filesystem::path traceFileName = {}) EXT_THROWS(std::runtime_error, std::filesystem::filesystem_error)
    {
        if (traceFileName.empty()) {
            auto tracesDirectory = std::filesystem::get_exe_directory().append("Traces");
            if (!std::filesystem::exists(tracesDirectory) && !std::filesystem::create_directories(tracesDirectory))
            {
                assert(false);
                throw std::runtime_error("Failed to create trace directory!");
            }

            const auto getLogName = []() -> std::string {
                using namespace std::chrono;

                const auto now = system_clock::now();
                const std::time_t t = system_clock::to_time_t(now);
            
                std::tm time{};
#if defined(_WIN32) || defined(__CYGWIN__) // windows
                localtime_s(&time, &t);
#else
                localtime_r(&t, &time);
#endif

                std::string buffer(80, '\0');
                size_t res = std::strftime(buffer.data(), buffer.size(), "_%d.%m.%Y_%H.%M.%S.log", &time);
                if (!res)
                    return "strftime error";
                buffer.resize(res);
                return std::filesystem::get_binary_name().replace_extension("").string() + buffer;
            };

            // %EXE_DIR%//Traces//%EXE_NAME%_%DATA%_%TIME%.log
            traceFileName = tracesDirectory.append(getLogName());
        }

        m_outputFile.open(traceFileName);
        if (!m_outputFile.is_open())
            throw std::runtime_error("Failed to open trace file!");
    }

    ~FileTracer()
    {
        if (m_outputFile.is_open())
            m_outputFile.close();
        else
            assert(false); // file must be opened on creation
    }

    // ITracer
    void Trace(Level /*level*/, const std::string& text) override
    {
        if (m_outputFile.is_open())
        {
            m_outputFile << text.c_str() << std::endl;
            m_outputFile.flush();

            assert(m_outputFile.good());
        }
        else
            assert(false); // file must be opened on creation
    }

private:
    std::ofstream m_outputFile;
};

#if defined(_WIN32) || defined(__CYGWIN__) // windows
struct OutputTracer : ::ext::ITracer
{
    // ITracer
    void Trace(Level /*level*/, const std::string& text) override
    {
        OutputDebugString(text.c_str());
        OutputDebugString("\n");
    }
};
#endif

struct CmdLineTracer : ::ext::ITracer
{
    // ITracer
    void Trace(Level level, const std::string& text) override
    {
        if (level == Level::eError)
            std::cerr << text.c_str() << std::endl;
        else
            std::cout << text.c_str() << std::endl;
    }
};

using DefaultTracersList =
    ::ext::mpl::list<
        FileTracer,
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        OutputTracer,
#endif
        CmdLineTracer>;

[[nodiscard]] inline std::list<std::shared_ptr<ITracer>> default_tracers()
{
    std::list<std::shared_ptr<ITracer>> tracers;
    DefaultTracersList::ForEachType([&](auto* type)
        {
            using Type = std::remove_pointer_t<decltype(type)>;
            tracers.emplace_back(std::make_shared<Type>());
        });
    return tracers;
}

} // namespace ext::tracer::details
