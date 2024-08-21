cmake_minimum_required(VERSION 3.15)

if(NOT MSVC)
  # Including uuid package
  find_package(PkgConfig REQUIRED)
  pkg_search_module(UUID REQUIRED uuid)
endif()

# Test sources
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Preserve folder structure for test source files
foreach(SOURCE_FILE ${SOURCE_FILES})
    get_filename_component(SOURCE_PATH "${SOURCE_FILE}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" SOURCE_GROUP "${SOURCE_PATH}")
    source_group("tests\\${SOURCE_GROUP}" FILES "${SOURCE_FILE}")
endforeach()

add_executable(ext_tests ${SOURCE_FILES})

# Includes
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/ext/*.h")
foreach(HEADER_FILE ${HEADER_FILES})
    file(RELATIVE_PATH HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../include/ext" "${HEADER_FILE}")
    get_filename_component(HEADER_GROUP "${HEADER_PATH}" PATH)
    source_group("include\\${HEADER_GROUP}" FILES "${HEADER_FILE}")
endforeach()
target_sources(ext_tests PRIVATE ${HEADER_FILES})
target_include_directories(ext_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

# Samples
file(GLOB_RECURSE SAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/samples/*.*")
foreach(SAMPLE_FILE ${SAMPLE_FILES})
    file(RELATIVE_PATH SAMPLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/samples" "${SAMPLE_FILE}")
    get_filename_component(SAMPLE_GROUP "${SAMPLE_PATH}" PATH)
    source_group("samples\\${SAMPLE_GROUP}" FILES "${SAMPLE_FILE}")
endforeach()
target_sources(ext_tests PRIVATE ${SAMPLE_FILES})

# Compile options
if(MSVC)
  target_compile_options(ext_tests PRIVATE /W4 /WX)
else()
  target_compile_options(ext_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Adding test private headers
target_include_directories(ext_tests PRIVATE samples)

# Linking third-party libraries
target_link_libraries(ext_tests PRIVATE ${UUID_LIBRARIES} gtest_int)
