cmake_minimum_required(VERSION 3.14)
project(imagedecoder C CXX ASM)

set(CMAKE_BUILD_TYPE Release)

add_library(imagedecoder SHARED
    java_stream.cpp
    java_wrapper.cpp
    java_objects.cpp
    decoder_jpeg.cpp
    decoder_png.cpp
    decoder_webp.cpp
    borders.cpp
    row_convert.cpp
)

include(libjpeg-turbo/libjpeg-turbo.cmake)
include(libpng/libpng.cmake)
include(libwebp/libwebp.cmake)

target_link_libraries(imagedecoder android jnigraphics log)
