cmake_minimum_required(VERSION 3.15)

project(ext LANGUAGES CXX)

include(FetchContent)

# Setting third-party libraries versions
set(GTEST_VERSION 1.12.0 CACHE STRING "Google test version")
set(PUGI_XML_VERSION v1.13 CACHE STRING "Pugi XML version")

# Enable C++17 language standard.
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

macro(ext_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow overriding the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

ext_option(EXT_USE_SYSTEM_GTEST "If ON, this project will use an installed gtest library. If none is found it will use the built-in one." OFF)
ext_option(EXT_USE_SYSTEM_PUGIXML "If ON, this project will use an installed pugixml library. If none is found it will use the built-in one." OFF)
ext_option(EXT_BUILD_TESTS "Set to ON to build ext tests." OFF)

# Add ext include libraries
include_directories(include)

if (EXT_BUILD_TESTS)
    # GTest
    if (EXT_USE_SYSTEM_GTEST)
        find_package(GTest REQUIRED)
        message(STATUS "Using system gtest.")
    endif()

    if (NOT GTEST_FOUND)
        message(STATUS "Not using system gtest, using built-in googletest project instead.")

        FetchContent_Declare(googletest
                             GIT_REPOSITORY https://github.com/google/googletest.git
                             GIT_TAG release-${GTEST_VERSION})

        FetchContent_GetProperties(googletest)
        if (NOT googletest_POPULATED)
            FetchContent_Populate(googletest)
            add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
        endif()

        add_library(gtest_int INTERFACE)
        target_link_libraries(gtest_int INTERFACE gtest gmock)
        target_include_directories(gtest_int INTERFACE ${googletest_SOURCE_DIR}/include)
        add_library(GTest::GTest ALIAS gtest_int)
    endif()

    # Group under the "tests/gtest" project folder in IDEs such as Visual Studio.
    set_property(TARGET gtest PROPERTY FOLDER "tests/gtest")
    set_property(TARGET gtest_main PROPERTY FOLDER "tests/gtest")

    # Pugi xml
    if (EXT_USE_SYSTEM_PUGIXML)
        find_package(pugixml REQUIRED)
        message(STATUS "Using system pugixml.")
    endif()

    if (NOT PUGIXML_FOUND)
        message(STATUS "Not using system pugixml, using built-in pugixml project instead.")
        FetchContent_Declare(pugixml
                             GIT_REPOSITORY https://github.com/zeux/pugixml.git
                             GIT_TAG ${PUGI_XML_VERSION})

        FetchContent_GetProperties(pugixml)
        if (NOT pugixml_POPULATED)
            FetchContent_Populate(pugixml)
            add_subdirectory(${pugixml_SOURCE_DIR} ${pugixml_BINARY_DIR})
        endif()
    endif()

    # Adding test libraries
    enable_testing()

    # Adding ext_tests project
    add_subdirectory(tests)

endif()
