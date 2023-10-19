cmake_minimum_required(VERSION 3.15)

# Adding tests files
file(GLOB test_SRC "*.cpp" "*/*.cpp")
add_executable(ext_tests ${test_SRC})

# Adding pugixml source file to fix linker problems
target_sources(ext_tests PRIVATE ${pugixml_SOURCE_DIR}/src/pugixml.cpp)

# Adding test private headers
target_include_directories(ext_tests PRIVATE samples)
# Adding wchar_t support
target_compile_definitions(ext_tests PRIVATE PUGIXML_WCHAR_MODE)

# Linking third-party libraries
target_link_libraries(ext_tests PRIVATE gtest_int pugixml-static)
