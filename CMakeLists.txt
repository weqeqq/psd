cmake_minimum_required(VERSION 3.23)

project(PSD VERSION 0.1.0 LANGUAGES CXX C)

add_library(psd)
add_library(psd::psd ALIAS psd)

target_sources(psd PUBLIC
  FILE_SET  HEADERS
  BASE_DIRS include
  FILES
)
target_sources(psd PRIVATE
    source/capi/document.cc
    source/capi/document/layer.cc
    source/capi/document/group.cc
    source/core/compressor/rle.cc
    source/core/decompressor/rle.cc
    source/structure/main_info/extra_info.cc
)

include(FetchContent)

FetchContent_Declare(
  image
  GIT_REPOSITORY https://github.com/weqeqq/image.git
  GIT_BRANCH psd-cpp
  GIT_TAG 205819e59e89878dd80e02783da9d82c449e7c07
)
FetchContent_Declare(
  xsimd
  GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
  GIT_TAG        13.2.0
)
FetchContent_MakeAvailable(
  image
  xsimd
)
target_link_libraries(psd PUBLIC
  image::image
  xsimd
)

set_target_properties(psd PROPERTIES
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED 1
  CXX_EXTENSIONS        0)

install(TARGETS psd)
install(DIRECTORY include DESTINATION include)
