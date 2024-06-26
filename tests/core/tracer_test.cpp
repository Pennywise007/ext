#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <cstdio>
#include <memory>
#include <list>

#include <ext/core/tracer.h>

using namespace testing;

struct TracerMock : ext::ITracer {

    MOCK_METHOD(void, Trace, (Level level, const std::string& text), (override));
};

struct TestFixture : Test {

    TestFixture()
    {
        ext::TraceManager::Settings defExtensions;
        ext::get_tracer().SetSettings(std::move(defExtensions));
        ext::get_tracer().Reset();
    }

    ~TestFixture()
    {
        ext::get_tracer().Reset();
    }

    void EnableTraces(ext::ITracer::Level level)
    {
        auto tracer = std::static_pointer_cast<ext::ITracer>(mock_);
        ext::get_tracer().Enable(level, {tracer});
    }

    std::shared_ptr<StrictMock<TracerMock>> mock_ = std::make_shared<StrictMock<TracerMock>>();
};

TEST_F(TestFixture, no_traces_when_tracer_reset)
{
    ext::get_tracer().Reset();
    EXT_TRACE_DBG() << "Debug, makarena";
    EXT_TRACE() << "Info makarena";
    EXT_TRACE_ERR() << "Error, makarena";
}

TEST_F(TestFixture, no_debug_tracing_in_info_level)
{
    EnableTraces(ext::ITracer::Level::eInfo);
    EXT_TRACE_DBG() << "Debug, makarena";
}

TEST_F(TestFixture, no_debug_or_info_tracing_in_err_level)
{
    EnableTraces(ext::ITracer::Level::eError);
    EXT_TRACE_DBG() << "Debug, makarena";
    EXT_TRACE() << "Info makarena";
}

TEST_F(TestFixture, full_tracing_for_debug_mode)
{
    EnableTraces(ext::ITracer::Level::eDebug);
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eDebug, _));
    EXT_TRACE_DBG() << "Debug, makarena";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eInfo, _));
    EXT_TRACE() << "Info makarena";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eError, _));
    EXT_TRACE_ERR() << "Error, makarena";
}

TEST_F(TestFixture, tracing_without_extensions)
{
    ext::TraceManager::Settings extensions;
    extensions.Extensions.reset();
    ext::get_tracer().SetSettings(std::move(extensions));

    EnableTraces(ext::ITracer::Level::eDebug);
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eDebug, StrEq("DBG\tDebug makarena")));
    EXT_TRACE_DBG() << "Debug makarena";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eInfo, StrEq("INF\tInfo makarena")));
    EXT_TRACE() << "Info makarena";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eError, StrEq("ERR\tError makarena")));
    EXT_TRACE_ERR() << "Error makarena";
}

TEST_F(TestFixture, tracing_thread_id)
{
    ext::TraceManager::Settings extensions;
    extensions.Extensions.reset();
    extensions.Extensions.set(ext::TraceManager::Settings::Extensions::eThreadId);
    ext::get_tracer().SetSettings(std::move(extensions));

    std::ostringstream threadIdText;
    threadIdText << "0x" << std::hex << std::this_thread::get_id() << "\t";

    EnableTraces(ext::ITracer::Level::eDebug);
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eDebug, StrEq(threadIdText.str() + "DBG\t" + "Trace text")));
    EXT_TRACE_DBG() << "Trace text";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eInfo, StrEq(threadIdText.str() + "INF\t" + "Trace text")));
    EXT_TRACE() << "Trace text";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eError, StrEq(threadIdText.str() + "ERR\t" + "Trace text")));
    EXT_TRACE_ERR() << "Trace text";
}

TEST_F(TestFixture, tracing_date)
{
    ext::TraceManager::Settings extensions;
    extensions.Extensions.reset();
    extensions.Extensions.set(ext::TraceManager::Settings::Extensions::eDate);
    ext::get_tracer().SetSettings(std::move(extensions));

    auto checkText = [](auto&&, const std::string& text) {
        char level[10], traceText[10];
        
        std::time_t localTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm traceTm{};
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        localtime_s(&traceTm, &localTime);
        EXPECT_EQ(5, sscanf_s(text.c_str(), "%d:%d:%d\t%s\t%s",
            &traceTm.tm_hour, &traceTm.tm_min, &traceTm.tm_sec,
            level, (unsigned int)sizeof(level),
            traceText, (unsigned int)sizeof(traceText)));
#else
        localtime_r(&localTime, &traceTm);
        EXPECT_EQ(5, sscanf(text.c_str(), "%d:%d:%d\t%s\t%s",
            &traceTm.tm_hour, &traceTm.tm_min, &traceTm.tm_sec,
            level, traceText));
#endif
        std::time_t traceTime = std::mktime(&traceTm);

        EXPECT_STREQ("Trace", traceText);
        ASSERT_LE(traceTime, localTime);
        EXPECT_LE(traceTime - localTime, 1);
    };

    EnableTraces(ext::ITracer::Level::eDebug);
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eDebug, _)).WillOnce(Invoke(checkText));
    EXT_TRACE_DBG() << "Trace";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eInfo,  _)).WillOnce(Invoke(checkText));
    EXT_TRACE() << "Trace";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eError,  _)).WillOnce(Invoke(checkText));
    EXT_TRACE_ERR() << "Trace";
}

TEST_F(TestFixture, default_tracing)
{
    auto checkText = [](auto&&, const std::string& text) {
        int miliseconds;
        char level[10], traceText[10], threadId[20];
        
        std::time_t localTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm traceTm{};
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        localtime_s(&traceTm, &localTime);

        EXPECT_EQ(7, sscanf_s(text.c_str(), "%d:%d:%d.%d\t%s\t%s\t%s",
            &traceTm.tm_hour, &traceTm.tm_min, &traceTm.tm_sec, &miliseconds,
            threadId, (unsigned int)sizeof(threadId),
            level, (unsigned int)sizeof(level),
            traceText, (unsigned int)sizeof(traceText)));
#else
        localtime_r(&localTime, &traceTm);

        EXPECT_EQ(7, sscanf(text.c_str(), "%d:%d:%d.%d\t%s\t%s\t%s",
            &traceTm.tm_hour, &traceTm.tm_min, &traceTm.tm_sec, &miliseconds,
            threadId, level, traceText));
#endif
        std::time_t traceTime = std::mktime(&traceTm);

        EXPECT_STREQ("Text", traceText);
        EXPECT_LE(miliseconds, 999);
        EXPECT_GE(miliseconds, 0);
        ASSERT_LE(traceTime, localTime);
        EXPECT_LE(traceTime - localTime, 1);
    };

    EnableTraces(ext::ITracer::Level::eDebug);
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eDebug, _)).WillOnce(Invoke(checkText));
    EXT_TRACE_DBG() << "Text";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eInfo,  _)).WillOnce(Invoke(checkText));
    EXT_TRACE() << "Text";
    EXPECT_CALL(*mock_, Trace(ext::ITracer::Level::eError,  _)).WillOnce(Invoke(checkText));
    EXT_TRACE_ERR() << "Text";
}
