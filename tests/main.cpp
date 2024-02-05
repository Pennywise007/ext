
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <ext/core.h>

int main(int argc, char **argv)
{
    if (IsDebuggerPresent())
        ::testing::FLAGS_gtest_catch_exceptions = false;

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    ext::core::Init();

    return RUN_ALL_TESTS();
}