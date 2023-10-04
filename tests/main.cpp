
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <ext/core.h>
#include <ext/trace/tracer.h>

int main(int argc, char **argv)
{
    ::testing::FLAGS_gtest_catch_exceptions = false;

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    ext::core::Init();
    ext::get_tracer()->EnableTraces(true);

    return RUN_ALL_TESTS();
}