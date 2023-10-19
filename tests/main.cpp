
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <ext/core.h>
#include <ext/core/tracer.h>

int main(int argc, char **argv)
{
    ::testing::FLAGS_gtest_catch_exceptions = false;

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    ext::core::Init();

    return RUN_ALL_TESTS();
}