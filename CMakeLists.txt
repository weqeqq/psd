cmake_minimum_required(VERSION 3.23)

project(psd C CXX)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(PSD_BUILD_TESTS       "" OFF)
# option(PSD_FETCH_FILE_CPP    "" ON)
# option(PSD_FETCH_UNICODE_CPP "" ON)

add_library(psd)
include(GenerateExportHeader)
generate_export_header(
    psd
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/headers/psd/export.h
)
target_sources(psd
    PUBLIC
        FILE_SET HEADERS BASE_DIRS
            headers
            ${CMAKE_CURRENT_BINARY_DIR}/headers
        FILES
            headers/psd/error.h
            ${CMAKE_CURRENT_BINARY_DIR}/headers/psd/export.h
    PRIVATE
        sources/capi/document/group.cc
        sources/capi/document/layer.cc
        sources/capi/document.cc
        sources/llapi/structure/info/layer_info/channel_data.cc
)
include(FetchContent)
FetchContent_Declare(
  dependency-cmake
  GIT_REPOSITORY https://github.com/weqeqq/dependency-cmake
  GIT_TAG        e982bc12360ac70cbef756c90c23cddb50c9d501
)
FetchContent_MakeAvailable(dependency-cmake)
FetchContent_Declare(
    file-cpp
    GIT_REPOSITORY https://github.com/weqeqq/file-cpp
    GIT_TAG 24ff1a4b1d94883ddd8f560c955df88388147270
)
FetchContent_MakeAvailable(file-cpp)
FetchContent_Declare(
    image-cpp
    GIT_REPOSITORY https://github.com/weqeqq/image-cpp
    GIT_TAG bc0c19a9c3fa7b223b6cdbfbda3fb35ac45c0a2b
)
set(IMAGE_DEPENDENCY_FILE_IMPORTED ON CACHE INTERNAL "")
FetchContent_MakeAvailable(image-cpp)
FetchContent_Declare(
  xsimd
  GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
  GIT_TAG        13.2.0
)
FetchContent_MakeAvailable(xsimd)
FetchContent_Declare(
    libdeflate
    GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
    GIT_TAG 96836d7d9d10e3e0d53e6edb54eb908514e336c4
)
set(LIBDEFLATE_BUILD_STATIC_LIB ON  CACHE INTERNAL "")
set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE INTERNAL "")
set(LIBDEFLATE_GZIP_SUPPORT     OFF CACHE INTERNAL "")
set(LIBDEFLATE_BUILD_GZIP       OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(libdeflate)

target_link_libraries(psd
    PUBLIC
        file::file
        image::image
        image::io
        libdeflate::libdeflate_static
        xsimd
)
if(CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    target_compile_definitions(psd PUBLIC PSD_LITTLE_ENDIAN)
endif()
add_library(psd::psd ALIAS psd)
if(PSD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS psd FILE_SET HEADERS)
