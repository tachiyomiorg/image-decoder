cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(CMAKE_BUILD_TYPE Release)

FetchContent_Declare(libjpeg-turbo
  GIT_REPOSITORY  https://github.com/libjpeg-turbo/libjpeg-turbo
  GIT_TAG         2.0.90
)

option(WITH_JPEG8 "" 1)
option(WITH_TURBOJPEG "" 0)
option(ENABLE_SHARED "" 0)
option(REQUIRE_SIMD "" 1)

FetchContent_MakeAvailable(libjpeg-turbo)

include_directories(${libjpeg-turbo_BINARY_DIR} ${libjpeg-turbo_SOURCE_DIR})
target_link_libraries(imagedecoder jpeg-static)
