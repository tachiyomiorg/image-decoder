cmake_minimum_required(VERSION 3.14)
project(imagedecoder C CXX ASM)

set(CMAKE_BUILD_TYPE Release)

option(WITH_JPEG "Include JPEG decoder" ON)
option(WITH_PNG "Include PNG decoder" ON)
option(WITH_WEBP "Include WebP decoder" ON)
option(WITH_HEIF "Include HEIF decoder" OFF)
option(WITH_AVIF "Include AVIF decoder" OFF)

add_library(imagedecoder SHARED
  java_stream.cpp
  java_wrapper.cpp
  java_objects.cpp
  borders.cpp
  row_convert.cpp
)

if(WITH_JPEG)
  add_subdirectory(libjpeg-turbo)
  add_definitions(-DHAVE_LIBJPEG)
  target_sources(imagedecoder PRIVATE decoder_jpeg.cpp)
endif()
if(WITH_PNG)
  add_subdirectory(libpng)
  add_definitions(-DHAVE_LIBPNG)
  target_sources(imagedecoder PRIVATE decoder_png.cpp)
endif()
if(WITH_WEBP)
  add_subdirectory(libwebp)
  add_definitions(-DHAVE_LIBWEBP)
  target_sources(imagedecoder PRIVATE decoder_webp.cpp)
endif()
if(WITH_HEIF OR WITH_AVIF)
  add_subdirectory(libheif)
  add_definitions(-DHAVE_LIBHEIF)
  target_sources(imagedecoder PRIVATE decoder_heif.cpp)
endif()

target_link_libraries(imagedecoder android jnigraphics log)
