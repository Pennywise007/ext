#pragma once

#include "gtest/gtest.h"

// Help functions for work with samples
#include <filesystem>
#include <fstream>
#include <string>

#include <ext/std/string.h>

namespace test::samples {

inline std::filesystem::path sample_file_path(const char* resource_file_name)
{
    const auto filePath = std::filesystem::path(__FILE__);
    return filePath.parent_path() / resource_file_name;
}

inline std::string load_sample_file(const char* resource_file_name)
{
    std::ifstream file(sample_file_path(resource_file_name));
    EXPECT_FALSE(file.fail()) << "Failed to open file " << resource_file_name;

    std::string fileText;
    std::copy(std::istreambuf_iterator<char>(file),
              std::istreambuf_iterator<char>(),
              std::insert_iterator<std::string>(fileText, fileText.begin()));
    return fileText;
}

// compare text with resource content
inline void expect_equal_to_sample(const std::wstring& text, const char* resource_file_name)
{
    auto sampleText = load_sample_file(resource_file_name);
    sampleText.erase(std::remove(sampleText.begin(), sampleText.end(), '\r'), sampleText.end());

    auto currentText = text;
    currentText.erase(std::remove(currentText.begin(), currentText.end(), L'\r'), currentText.end());
    EXPECT_STREQ(sampleText.c_str(), std::narrow(currentText).c_str()) << "Content unmatched with sample file " << resource_file_name;
}

// compare file with resource
inline void expect_equal_to_sample(const std::filesystem::path& path_to_file, const char* resource_file_name)
{
    std::wifstream file(path_to_file);
    ASSERT_FALSE(file.fail()) << "Failed to open file " << path_to_file;

    // load file content
    std::wstring fileText;
    std::copy(std::istreambuf_iterator<wchar_t>(file),
              std::istreambuf_iterator<wchar_t>(),
              std::insert_iterator<std::wstring>(fileText, fileText.begin()));

    expect_equal_to_sample(fileText, resource_file_name);
}

} // namespace test::samples
