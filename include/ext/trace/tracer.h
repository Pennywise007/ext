/*
* Implementation of ITracer and ScopedCallTracer
* Allow to trace text to output/file
* Trace file location : %EXE_PATH%/trace/%EXENAME%_%DATATIME_OF_ENABLE_TRACES%.log
*/
#pragma once

#include <atomic>
#include <algorithm>
#include <cassert>
#include <cwctype>
#include <exception>
#include <optional>
#include <string>
#include <sstream>
#include <stdio.h>
#include <mutex>
#include <list>
#include <thread>

#include <ctime>

#include <ext/trace/itracer.h>

#include <ext/core/defines.h>
#include <ext/core/singleton.h>
#include <ext/core/mpl.h>

#include <ext/error/exception.h>
#include <ext/std/string.h>

// Internal macro to call scope tracer, allow adding extra text to stream after call
#define EXT_TRACE_SCOPE_TYPE_EX(traceType, name)                                \
        ::ext::trace::ScopedCallTracer<traceType> name;                         \
        while (name.CanTrace() && !name)                                        \
            for (std::wostringstream stream; !name;)                            \
                for (bool streamSetted = false; !name; streamSetted = true)     \
                    if (streamSetted)                                           \
                        name.SetData(stream.str());                             \
                    else                                                        \
                        stream

/* Trace text inside scope, will show text on begin and end of current scope */
#define EXT_TRACE_SCOPE_TYPE(traceType) EXT_TRACE_SCOPE_TYPE_EX(traceType, EXT_PP_CAT(__scoped_tracer_, __COUNTER__))

/* Trace text inside scope, will show text on begin and end of current scope
Usage EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "my trace"; */
#define EXT_TRACE_SCOPE()       EXT_TRACE_SCOPE_TYPE(::ext::ITracer::Type::eNormal)

/* Trace text in DBG inside scope, will show text on begin and end of current scope
Usage EXT_TRACE_SCOPE_DBG() << EXT_TRACE_FUNCTION << "my trace";*/
#define EXT_TRACE_SCOPE_DBG()   EXT_TRACE_SCOPE_TYPE(::ext::ITracer::Type::eDebug)

#include <ext/trace/workers.h>

namespace ext {
namespace trace {

template <typename StreamType>
inline StreamType& operator<<(StreamType& stream, const ::ext::ITracer::Type type)
{
    switch (type)
    {
    case ::ext::ITracer::Type::eDebug:
        return stream << "DBG";
    case ::ext::ITracer::Type::eError:
        return stream << "ERR";
    case ::ext::ITracer::Type::eNormal:
        return stream << "INF";
    default:
        return stream << "UNKNOWN";
    }
}

/*
* Thread safe singleton realization of tracer, allow trace text to output and file
*/
class Tracer : public ::ext::ITracer
{
    friend ext::Singleton<Tracer>;

private:
    bool CanTrace(const ::ext::ITracer::Type type) const noexcept override
    {
#ifndef _DEBUG
        if (type == ::ext::ITracer::Type::eDebug)
            return false;
#else
        EXT_UNUSED(type);
#endif // _DEBUG

        return m_enable;
    }

    void Trace(const ::ext::ITracer::Type type, std::string text) noexcept override
    try
    {
        EXT_DUMP_IF(text.empty()) << "empty trace text";

        if (!CanTrace(type))
            return;

        constexpr auto trimTextRight = [](std::string& text)
        {
            const auto it = std::find_if(text.rbegin(), text.rend(), [](char ch) { return !std::isspace(ch); });
            text.erase(it.base(), text.end());
        };
        trimTextRight(text);

        std::ostringstream outputStringPrefix;

        struct timespec currentTime;
        timespec_get(&currentTime, TIME_UTC);
        outputStringPrefix << std::string_sprintf("%02d:%02d:%02d.%03d",
                                                  (currentTime.tv_sec % 86400) / 3600,
                                                  (currentTime.tv_sec % 3600) / 60,
                                                  currentTime.tv_sec % 60,
                                                  currentTime.tv_nsec / 1000000);

        outputStringPrefix << '\t' << std::hex << std::this_thread::get_id() << '\t' << type << '\t';

        text = outputStringPrefix.str() + std::move(text);

        const auto* str = text.c_str();

        std::scoped_lock traceLock(m_tracersMutex);
        for (auto& tracer : m_tracers)
        {
            tracer->Trace(type, str);
        }
    }
    catch (const std::exception&)
    {
        ext::ManageException(EXT_TRACE_FUNCTION);
    }

    void EnableTraces(const bool enable) noexcept override
    try
    {
        if (m_enable == enable)
            return;
        m_enable = enable;

        std::scoped_lock traceLock(m_tracersMutex);
        if (m_enable)
        {
            workers::TracerWorkersList::ForEachType(
                [tracers = &m_tracers](auto* worker)
                {
                    try
                    {
                        tracers->emplace_back(std::make_unique<std::remove_pointer_t<decltype(worker)>>());
                    }
                    catch (const std::exception&)
                    {}
                });
        }
        else
            m_tracers.clear();
    }
    catch (const std::exception&)
    {
        ext::ManageException(EXT_TRACE_FUNCTION);
    }

private:
    std::atomic_bool m_enable = false;

    std::mutex m_tracersMutex;
    std::list<std::unique_ptr<workers::ITraceWorker>> m_tracers;
};

/*
* Special class to trace scope enter and exit, Trace "%TRACE_TEXT% << begin" on SetData and "%TRACE_TEXT% << end" on scope exit
* Do nothing if data not setted
*/
template<::ext::ITracer::Type traceType>
struct ScopedCallTracer final : ::ext::NonCopyable
{
    ~ScopedCallTracer()
    {
        if (m_text.has_value())
            EXT_TRACE_TYPE(traceType) << m_text.value() << " end";
    }

    void SetData(std::wstring&& text)
    {
        m_text = std::move(text);
        EXT_TRACE_TYPE(traceType) << m_text.value() << " begin";
    }

    bool CanTrace() const noexcept { return m_tracer->CanTrace(traceType); }
    bool operator!() const noexcept { return !m_text.has_value(); }

private:
    ::ext::ITracer* m_tracer = ::ext::get_tracer();
    std::optional<std::wstring> m_text;
};

} // namespace trace

// global function for getting tracer
EXT_NODISCARD inline ::ext::ITracer* get_tracer()
{
    return static_cast<::ext::ITracer*>(&get_service<::ext::trace::Tracer>());
}

} // namespace ext
