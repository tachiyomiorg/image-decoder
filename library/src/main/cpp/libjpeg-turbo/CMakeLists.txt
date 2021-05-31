include(FetchContent)

FetchContent_Declare(libjpeg-turbo
  GIT_REPOSITORY  https://github.com/libjpeg-turbo/libjpeg-turbo
  GIT_TAG         2.0.90
  PATCH_COMMAND   git apply
    ${CMAKE_CURRENT_LIST_DIR}/01-decode-cmyk-to-rgb.patch
    ${CMAKE_CURRENT_LIST_DIR}/02-decode-ycck-to-rgb.patch
    ${CMAKE_CURRENT_LIST_DIR}/03-decode-cmyk-to-rgb565.patch
    ${CMAKE_CURRENT_LIST_DIR}/04-decode-ycck-to-rgb565.patch || true
)

option(WITH_JPEG8 "" 1)
option(WITH_TURBOJPEG "" 0)
option(ENABLE_SHARED "" 0)
option(REQUIRE_SIMD "" 1)

FetchContent_MakeAvailable(libjpeg-turbo)

target_include_directories(imagedecoder PRIVATE ${libjpeg-turbo_BINARY_DIR} ${libjpeg-turbo_SOURCE_DIR})
target_link_libraries(imagedecoder jpeg-static)
