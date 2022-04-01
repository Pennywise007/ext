#pragma once

#include <debugapi.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <atlstr.h>
#include <atltime.h>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>
#include <ext/core/mpl.h>
#include <ext/error/dump_writer.h>
#include <ext/std/filesystem.h>
#include <ext/trace/itracer.h>

namespace ext::trace::workers {

struct ITraceWorker : ext::NonCopyable
{
    virtual ~ITraceWorker() = default;
    virtual void Trace(const ::ext::ITracer::Type type, const char* text) noexcept = 0;
};

struct FileTracer : ITraceWorker
{
    FileTracer() EXT_THROWS(std::runtime_error, std::filesystem::filesystem_error)
    {
        auto tracesDirectory = std::filesystem::get_exe_directory().append("Traces");
        if (!std::filesystem::exists(tracesDirectory) && !std::filesystem::create_directories(tracesDirectory))
        {
            DEBUG_BREAK_OR_CREATE_DUMP();
            throw std::runtime_error("Failed to create trace directory!");
        }

        // %EXE_DIR%//Traces//%EXE_NAME%_%DATA%_%TIME%.log
        const auto traceFileName = tracesDirectory
            .append(std::filesystem::get_exe_name().string())
            .append(CTime::GetCurrentTime().Format(L"_%d.%m.%Y_%H.%M.%S.log").GetString());

        if (std::filesystem::exists(traceFileName))
            DEBUG_BREAK_OR_CREATE_DUMP(); // it must be first file creation

        m_outputFile.open(traceFileName);
        if (!m_outputFile.is_open())
            throw std::runtime_error("Failed to open trace file!");
    }

    ~FileTracer()
    {
        if (m_outputFile.is_open())
            m_outputFile.close();
        else
            DEBUG_BREAK_OR_CREATE_DUMP(); // file must be opened on creation class
    }

    // ITraceWorker
    void Trace(const ::ext::ITracer::Type /*type*/, const char* text) noexcept override
    {
        if (m_outputFile.is_open())
        {
            m_outputFile << text << std::endl;
            m_outputFile.flush();

            EXT_DUMP_IF(!m_outputFile.good()) << EXT_TRACE_FUNCTION;
        }
        else
            DEBUG_BREAK_OR_CREATE_DUMP(); // file must be opened on creation class
    }

private:
    std::ofstream m_outputFile;
};

struct OutputTracer : ITraceWorker
{
    // ITraceWorker
    void Trace(const ::ext::ITracer::Type /*type*/, const char* text) noexcept override
    {
        OutputDebugStringA(text);
        OutputDebugStringA("\n");
    }
};

struct CmdLineTracer : ITraceWorker
{
    // ITraceWorker
    void Trace(const ::ext::ITracer::Type type, const char* text) noexcept override
    {
        if (type == ::ext::ITracer::Type::eError)
            std::cerr << text << std::endl;
        else
            std::cout << text << std::endl;
    }
};

using TracerWorkersList = ::ext::mpl::list<FileTracer, OutputTracer, CmdLineTracer>;

} // namespace ext::trace::workers
