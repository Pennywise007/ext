cmake_minimum_required(VERSION 3.15)

if(NOT MSVC)
  # Including uuid package
  find_package(PkgConfig REQUIRED)
  pkg_search_module(UUID REQUIRED uuid)
endif()

# Adding tests files
file(GLOB test_SRC "*.cpp" "*/*.cpp")
add_executable(ext_tests ${test_SRC})

# Adding compile time checks
if(MSVC)
  target_compile_options(ext_tests PRIVATE /W4 /WX)
else()
  target_compile_options(ext_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Adding pugixml source file to fix linker problems
target_sources(ext_tests PRIVATE ${pugixml_SOURCE_DIR}/src/pugixml.cpp)

# Adding test private headers
target_include_directories(ext_tests PRIVATE samples)
# Adding wchar_t support
target_compile_definitions(ext_tests PRIVATE PUGIXML_WCHAR_MODE)

# Linking third-party libraries
target_link_libraries(ext_tests PRIVATE ${UUID_LIBRARIES} gtest_int pugixml-static)
