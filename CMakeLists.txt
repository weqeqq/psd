cmake_minimum_required(VERSION 3.23)

project(psd C CXX)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

add_library(psd)
target_sources(psd PUBLIC
    FILE_SET HEADERS BASE_DIRS
        headers
    FILES
        headers/psd/psd.h
        headers/psd/error.h
    PRIVATE
        sources/file.cc
)
find_package(file REQUIRED)
target_link_libraries(psd
    PUBLIC
        file::file
)
target_compile_features(psd PUBLIC cxx_std_17)
add_library(psd::psd ALIAS psd)

option(PSD_BUILD_TESTS "" ON)
if(PSD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
