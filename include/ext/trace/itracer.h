/*
* Interface of ITracer, don`t forget to include implementation via
* #include <core/trace/tracer.h>
*/

#pragma once

#include <sstream>
#include <string>

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#define EXT_FUNCTION __FUNCTION__

// Macro for tracing current function, basically used in trace prefix
#define EXT_TRACE_FUNCTION "[" EXT_FUNCTION "(line " __LINE__ ")]: "
#elif defined(__GNUC__) // linux
inline std::string method_name (const std::string &fsig)
{
  size_t colons = fsig.find ("::");
  size_t sbeg = fsig.substr (0, colons).rfind (" ") + 1;
  size_t send = fsig.rfind ("(") - sbeg;
  return fsig.substr (sbeg, send) + "()";
}

#define EXT_FUNCTION method_name(__PRETTY_FUNCTION__)

// Macro for tracing current function, basically used in trace prefix
#define EXT_TRACE_FUNCTION ("[" + EXT_FUNCTION + "(line "+ std::to_string(__LINE__) + ")]: ").c_str()

#endif

// Default trace macros, using: EXT_TRACE_TYPE(::ext::ITracer::TraceType::eNormal) << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_TYPE(TraceType) \
    for (auto* __tracer = ::ext::get_tracer(); __tracer && __tracer->CanTrace(TraceType); __tracer = nullptr)   \
        if (std::ostringstream __stream; __tracer)                                                              \
            for (bool __streamIsSet = false; __tracer; __streamIsSet = true)                                    \
                if (__streamIsSet)                                                                              \
                {                                                                                               \
                    __tracer->Trace(TraceType, __stream.str());                                                 \
                    __tracer = nullptr;                                                                         \
                }                                                                                               \
                else                                                                                            \
                    __stream

// Default trace macros, using: EXT_TRACE() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE()         EXT_TRACE_TYPE(::ext::ITracer::Type::eNormal)
// Trace macros for debug output, using: EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_DBG()     EXT_TRACE_TYPE(::ext::ITracer::Type::eDebug)
// Trace macros for errors, using: EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "my text";
#define EXT_TRACE_ERR()     EXT_TRACE_TYPE(::ext::ITracer::Type::eError)

namespace ext {

/*
* ITracer interface for tracer realization
*/
struct ITracer
{
    virtual ~ITracer() = default;

    enum class Type
    {
        eDebug, // Tracing only in debug configuration
        eError, // Trace error, using cerr output and special markers
        eNormal // Ordinary trace
    };

    // Check if tracer works in given mode
    virtual bool CanTrace(const ITracer::Type type) const noexcept = 0;
    // Trace text in given trace mode
    virtual void Trace(const ITracer::Type type, std::string text) noexcept = 0;
    // Enable/disable tracer
    virtual void EnableTraces(const bool enable) noexcept = 0;
};

// #include <ext/tracer/tracer.h>
// global function for getting tracer
inline ::ext::ITracer* get_tracer();

} // namespace ext
