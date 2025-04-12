cmake_minimum_required(VERSION 3.23)

project(PSD VERSION 0.1.0 LANGUAGES CXX)

add_library(psd)
add_library(psd::psd ALIAS psd)

target_sources(psd PUBLIC 
  FILE_SET  HEADERS 
  BASE_DIRS include 
  FILES 
)
target_sources(psd PRIVATE 
  sources/compressor/rle.cc
  # sources/compressor/zip.cc
  # sources/compressor/zipp.cc
  sources/decompressor/rle.cc
  # sources/decompressor/zip.cc
  # sources/decompressor/zipp.cc
)

include(FetchContent)

FetchContent_Declare(
  image 
  GIT_REPOSITORY https://github.com/weqeqq/image.git
  GIT_TAG        eaffbc83ef5db75251a02de1fa532117fe6dfdcd
)

FetchContent_MakeAvailable(image)

target_link_libraries(psd PUBLIC image::image)

set_target_properties(psd PROPERTIES 
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED 1
  CXX_EXTENSIONS        0)

target_compile_options(psd PRIVATE -O3 -Wall -Wpedantic)
