/*
Tracer for data. Examples:

- Default information trace
	EXT_TRACE() << "My trace"; 
- Debug information only for Debug build
	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION "called";
- Error trace to cerr, mostly used in EXT_CHECK/EXT_EXPECT
	EXT_TRACE_ERR() << EXT_TRACE_FUNCTION "called";

Can be called for scope call function check. Trace start and end scope with the given text
	EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "Main function called with " << args;
*/

#pragma once

#include <cassert>
#include <bitset>
#include <list>
#include <memory>
#include <chrono>
#include <thread>
#include <optional>
#include <sstream>
#include <mutex>
#include <string>

#include <stdio.h>
#include <time.h>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>
#include <ext/std/string.h>         // to make operator<< for strings visible

// Macro for tracing current function, basically used in trace prefix
#define EXT_TRACE_FUNCTION (std::string("[") + EXT_FUNCTION + "(line " + std::to_string(__LINE__) + ")]: ").c_str()

// Default trace macros, usage: EXT_TRACE_TYPE(::ext::ITracer::TraceType::eInfo) << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_LEVEL(__Level)                                                                                \
    for (auto* __tracer = &::ext::get_tracer(); __tracer && __tracer->CanTrace(__Level); __tracer = nullptr)    \
        if (std::ostringstream __stream; !__tracer)                                                             \
        {}                                                                                                      \
        else                                                                                                    \
            for (bool __streamIsSet = false; __tracer; __streamIsSet = true)                                    \
                if (__streamIsSet)                                                                              \
                {                                                                                               \
                    __tracer->Trace(__Level, __stream.str());                                                   \
                    __tracer = nullptr;                                                                         \
                }                                                                                               \
                else                                                                                            \
                    __stream << ""

// Default trace macros, example:           EXT_TRACE() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE()         EXT_TRACE_LEVEL(::ext::ITracer::Level::eInfo)
// Trace macros for debug output, example:  EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_DBG()     EXT_TRACE_LEVEL(::ext::ITracer::Level::eDebug)
// Trace macros for errors, example:        EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_ERR()     EXT_TRACE_LEVEL(::ext::ITracer::Level::eError)

namespace ext {

struct ITracer
{
    enum class Level
    {
        eDebug, // Debug traces
        eInfo,  // Ordinary trace
        eError  // Trace error, using cerr output and special markers
    };

    virtual ~ITracer() = default;
    // Trace text in given trace mode
    virtual void Trace(Level level, const std::string& text) = 0;
};

namespace tracer::details {
// Get default trasers list
// defined in: ext/details/tracer_details.h
[[nodiscard]] std::list<std::shared_ptr<ITracer>> default_tracers(); 
} // tracer::details

/*
* Manager for all tracers, got a tracers list and send traces to all of them
  Supports text customization
*/
class TraceManager : public ITracer
{
    TraceManager() = default;
    TraceManager(TraceManager const&) = delete;
    TraceManager& operator= (TraceManager const&) = delete;

public:
    static TraceManager& Instance()
    {
        static TraceManager manager;
        return manager;
    }

    // Trace text in given trace mode with applying trace extensions
    void Trace(Level level, const std::string& text) override;
    
    // Enable traces and stop Add custom tracer
    void Enable(Level level = Level::eDebug,
                std::list<std::shared_ptr<ITracer>> tracers = tracer::details::default_tracers()) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
        tracers_ = std::move(tracers);
    }

    // Clear current tracers list and disable tracing
    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        level_.reset();
        tracers_.clear();
    }

    // Check if tracer works in given mode
    bool CanTrace(ITracer::Level level) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!level_.has_value())
            return false;
        return level >= level_.value();
    }

    // Trace extensions, applied to trace text
    struct Settings
    {
        enum Extensions
        {
            // Add date as trace prefix, you can set date format in DateFormat field
            eDate = 0,
            // Add date as trace prefix with milliseconds in the end, you can set date format in DateFormat field
            eDateWithMilliseconds,
            // Add current thread id
            eThreadId,
            // Extensions count
            eCount
        };

        // Format for date output, will be used with eDate flag
        // see https://en.cppreference.com/w/cpp/chrono/c/strftime for format details  
        std::string DateFormat = "%H:%M:%S";

        // Enabled extensions list
        std::bitset<size_t(Extensions::eCount)> Extenstions;
        Settings()
        {
            Extenstions.set(Extensions::eDateWithMilliseconds);
            Extenstions.set(Extensions::eThreadId);
        }
    };
    // Set tracer settings
    void SetSettings(Settings&& settings) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        settings_ = std::move(settings);
    }

private:
    std::string getTime()
    {
        if (!settings_.Extenstions.test(Settings::Extensions::eDate) &&
            !settings_.Extenstions.test(Settings::Extensions::eDateWithMilliseconds))
            return {};

        using namespace std::chrono;

        const auto now = system_clock::now();
        const std::time_t t = system_clock::to_time_t(now);

        std::string res(100, '\0');
        const size_t len = std::strftime(res.data(), res.size(), settings_.DateFormat.c_str(), std::localtime(&t));
        if (!len)
            return "strftime error";
        res.resize(len);

        if (settings_.Extenstions.test(Settings::Extensions::eDateWithMilliseconds))
            res += "." + std::to_string(std::chrono::duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000);
        return res + "\t";
    }

private:
    Settings settings_;
    std::mutex mutex_;
    std::optional<Level> level_;
    std::list<std::shared_ptr<ITracer>> tracers_;
};

// Global function for getting tracer
inline TraceManager& get_tracer()
{
    return TraceManager::Instance();
}

// Internal macro to call scope tracer, allow adding extra text to stream after call
#define EXT_TRACE_SCOPE_TYPE_EX(__level, __name)                            \
    ::ext::ScopedCallTracer<__level> __name;                                \
    while (__name.CanTrace() && !__name)                                    \
        for (std::wostringstream stream; !__name;)                          \
            for (bool streamSetted = false; !__name; streamSetted = true)   \
                if (streamSetted)                                           \
                    __name.SetData(stream.str());                           \
                else                                                        \
                    stream

// Trace text inside scope, will show text on begin and end of current scope
#define EXT_TRACE_SCOPE_TYPE(traceType) EXT_TRACE_SCOPE_TYPE_EX(traceType, EXT_PP_CAT(__scoped_tracer_, __COUNTER__))

/* Trace text inside scope, will show text on begin and end of current scope
Usage: EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "my trace"; */
#define EXT_TRACE_SCOPE()       EXT_TRACE_SCOPE_TYPE(::ext::ITracer::Level::eInfo)

/* Trace text in DBG inside scope, will show text on begin and end of current scope
Usage: EXT_TRACE_SCOPE_DBG() << EXT_TRACE_FUNCTION << "my trace";*/
#define EXT_TRACE_SCOPE_DBG()   EXT_TRACE_SCOPE_TYPE(::ext::ITracer::Level::eDebug)

/*
* Special class to trace scope enter and exit, Trace "%TRACE_TEXT% << begin" on SetData and "%TRACE_TEXT% << end" on scope exit
* Do nothing if data not setted
*/
template<::ext::ITracer::Level traceLevel>
struct ScopedCallTracer final : ::ext::NonCopyable
{
    ~ScopedCallTracer()
    {
        Trace("end");
    }

    void SetData(std::wstring&& text)
    {
        text_ = std::move(text);
        Trace("begin");
    }

    bool CanTrace() const noexcept { return tracer_.CanTrace(traceLevel); }
    bool operator!() const noexcept { return !text_.has_value(); }

private:
    void Trace(const char* suffix) const
    {
        if (!text_.has_value())
            return;
        const auto& outputText = text_.value();
        EXT_TRACE_LEVEL(traceLevel) << std::narrow(outputText).c_str() << (outputText.empty() ? "" : " ") << suffix;
    }

private:
    TraceManager& tracer_ = get_tracer();
    std::optional<std::wstring> text_;
};

template <typename StreamType>
inline StreamType& operator<<(StreamType& stream, const ::ext::ITracer::Level level)
{
    switch (level)
    {
    case ::ext::ITracer::Level::eDebug:
        stream << "DBG";
        break;
    case ::ext::ITracer::Level::eInfo:
        stream << "INF";
        break;
    case ::ext::ITracer::Level::eError:
        stream << "ERR";
        break;
    default:
        stream << "UNKNOWN";
        break;
    }
    return stream;
}

inline void TraceManager::Trace(Level level, const std::string& text)
{
    if (!CanTrace(level))
    {
        assert(false);
        return;
    }

    constexpr auto trimTextRight = [](const std::string& text)
    {
        const auto it = std::find_if(text.rbegin(), text.rend(), [](char ch) { return !std::isspace(ch); });
        return std::string_view(text.c_str(), std::distance(text.begin(), it.base()));
    };

    std::ostringstream traceText;
    traceText << getTime();    
    if (settings_.Extenstions.test(Settings::Extensions::eThreadId))
        traceText << "0x" << std::hex << std::this_thread::get_id() << '\t';
    traceText << level << "\t" << trimTextRight(text);
    const auto str = traceText.str();
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& tracer : tracers_)
    {
        tracer->Trace(level, str);
    }
}

} // namespace ext

#include <ext/details/tracer_details.h>
