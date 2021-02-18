cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(CMAKE_BUILD_TYPE Release)

FetchContent_Declare(libwebp
  GIT_REPOSITORY  https://chromium.googlesource.com/webm/libwebp
  GIT_TAG         v1.1.0
)

option(WEBP_BUILD_ANIM_UTILS "" OFF)
option(WEBP_BUILD_CWEBP "" OFF)
option(WEBP_BUILD_DWEBP "" OFF)
option(WEBP_BUILD_GIF2WEBP "" OFF)
option(WEBP_BUILD_IMG2WEBP "" OFF)
option(WEBP_BUILD_VWEBP "" OFF)
option(WEBP_BUILD_WEBPINFO "" OFF)
option(WEBP_BUILD_WEBPMUX "" OFF)
option(WEBP_BUILD_EXTRAS "" OFF)
option(WEBP_BUILD_WEBP_JS "" OFF)
option(WEBP_ENABLE_SWAP_16BIT_CSP "" ON)

FetchContent_MakeAvailable(libwebp)

include_directories(${libwebp_BINARY_DIR} ${libwebp_SOURCE_DIR})
target_link_libraries(imagedecoder webpdecoder)
