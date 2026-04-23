#include <gtest/gtest.h>
#include <optional>
#include <ext/core/command_line.h>

namespace {

struct TestFlags
{
    int port;
    bool verbose;
    std::optional<std::wstring> name;
};

ext::core::CommandLineParser::FlagInfo CreateFlagInfo(
    std::wstring_view alias, std::wstring_view summary, std::wstring_view description, bool required)
{
    ext::core::CommandLineParser::FlagInfo info;
    info.alias = alias;
    info.summary = summary;
    info.description = description;
    info.required = required;
    return info;
}

} // namespace

TEST(command_line_parser, parse_objects)
{
    const wchar_t* argv[] = {L"app", L"--port", L"8080", L"--verbose", L"--name", L"tester"};
    ext::core::CommandLineParser parser(6, argv);

    TestFlags flags{0, false, std::nullopt};
    parser.GetFlags(flags);

    EXPECT_EQ(flags.port, 8080);
    EXPECT_TRUE(flags.verbose);
    ASSERT_TRUE(flags.name.has_value());
    EXPECT_STREQ(flags.name.value().c_str(), L"tester");

    EXPECT_STREQ(parser.GetHelp().c_str(), L"Available flags are not registered.");
}

TEST(command_line_parser, alias_support_for_flag_names)
{
    const wchar_t* argv[] = {L"app", L"-p", L"8080", L"--verbose"};
    ext::core::CommandLineParser parser(4, argv);
    parser.RegisterFlag(L"port", CreateFlagInfo(L"p", L"Server port", L"Server listening port.", true));
    parser.RegisterFlag(L"verbose", CreateFlagInfo(L"v", L"Verbose logging", L"Enable additional log output.", false));

    int port = 0;
    bool verbose = false;
    EXPECT_TRUE(parser.GetFlag(L"p", port));
    EXPECT_EQ(port, 8080);
    EXPECT_TRUE(parser.GetFlag(L"port", port));
    EXPECT_EQ(port, 8080);
    EXPECT_TRUE(parser.GetFlag(L"verbose", verbose));
    EXPECT_TRUE(verbose);
    EXPECT_TRUE(parser.GetFlag(L"v", verbose));
    EXPECT_TRUE(verbose);
}

TEST(command_line_parser, generated_help_includes_alias_and_description)
{
    const wchar_t* argv[] = {L"app"};
    ext::core::CommandLineParser parser(1, argv);
    parser.RegisterFlag(L"port", CreateFlagInfo(L"p", L"Server port", L"Server listening port.", true));
    parser.RegisterFlag(L"verbose", CreateFlagInfo(L"v", L"Verbose logging", L"Enable additional log output.", false));

    const std::wstring help = parser.GetHelp();
    EXPECT_STREQ(help.c_str(), L"Required flags:\n\
  --port, -p\tServer port: Server listening port.\n\
\n\
Optional flags:\n\
  --verbose, -v\tVerbose logging: Enable additional log output.\n");
}

TEST(command_line_parser, custom_help_overrides_generated_help)
{
    const wchar_t* argv[] = {L"app"};
    ext::core::CommandLineParser parser(1, argv);
    parser.RegisterFlag(L"port", CreateFlagInfo(L"p", L"Server port", L"Server listening port.", true));
    parser.RegisterHelp(L"Custom usage message");

    EXPECT_STREQ(parser.GetHelp().c_str(), L"Custom usage message");
}

TEST(command_line_parser, throws_when_required_flag_missing)
{
    const wchar_t* argv[] = {L"app", L"--verbose"};
    ext::core::CommandLineParser parser(2, argv);
    parser.RegisterFlag(L"port", CreateFlagInfo(L"p", L"Server port", L"Required server port.", true));

    TestFlags flags{0, false, std::nullopt};
    EXPECT_THROW(parser.GetFlags(flags), std::runtime_error);
}

TEST(command_line_parser, bool_and_numeric_flag_parsing)
{
    const wchar_t* argv[] = {L"app", L"-v", L"--timeout", L"0", L"--enabled"};
    ext::core::CommandLineParser parser(5, argv);
    parser.RegisterFlag(L"enabled", CreateFlagInfo(L"e", L"Enable feature", L"Enable feature flag.", false));
    parser.RegisterFlag(L"timeout", CreateFlagInfo(L"t", L"Timeout seconds", L"Timeout in seconds.", true));

    int timeout = -1;
    bool enabled = false;
    EXPECT_TRUE(parser.GetFlag(L"timeout", timeout));
    EXPECT_EQ(timeout, 0);
    EXPECT_TRUE(parser.GetFlag(L"enabled", enabled));
    EXPECT_TRUE(enabled);
}
