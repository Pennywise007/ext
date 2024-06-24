#include "gtest/gtest.h"

#include <ext/core/check.h>
#include <ext/std/string.h>
#include <ext/error/exception.h>

TEST(exception_test, check_nested)
{
    try
    {
        try
        {
            throw ext::exception("Failed to do sth");
        }
        catch (...)
        {
            try
            {
                std::throw_with_nested(ext::exception(std::source_location::current(), "Job failed"));
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error("Runtime error"));
            }
        }
    }
    catch (const std::exception&)
    {
        EXPECT_STREQ(("Main error caught.\n\n"
                      "Exception: Runtime error\n" +
                      std::string_sprintf("Job failed Exception At '%s'(18).\n", std::source_location::current().function_name()) +
                      "Failed to do sth Exception").c_str(), ext::ManageExceptionText("Main error caught").c_str());
    }
}