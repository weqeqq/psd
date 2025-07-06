cmake_minimum_required(VERSION 3.23)

project(psd C CXX)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(PSD_BUILD_TESTS       "" ON)
option(PSD_FETCH_FILE_CPP    "" ON)
option(PSD_FETCH_UNICODE_CPP "" ON)

add_library(psd)
target_sources(psd PUBLIC
    FILE_SET HEADERS BASE_DIRS
        headers
    FILES
        headers/psd/error.h
    PRIVATE
        sources/file.cc
)
include(FetchContent)
if(PSD_FETCH_FILE_CPP)
    FetchContent_Declare(
        file-cpp
        GIT_REPOSITORY https://github.com/weqeqq/file-cpp.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(file-cpp)
else()
    find_package(file REQUIRED)
endif()
if(PSD_FETCH_UNICODE_CPP)
    # FetchContent_Declare(
    #     unicode-cpp
    #     GIT_REPOSITORY https://github.com/weqeqq/unicode-cpp.git
    #     GIT_TAG main
    # )
    # FetchContent_MakeAvailable(unicode-cpp)
else()
    message(FATAL_ERROR "Doesn't supported yet")
endif()
target_link_libraries(psd
    PUBLIC
        file::file
        # unicode::unicode
)
add_library(psd::psd ALIAS psd)
if(PSD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
