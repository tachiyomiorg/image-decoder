FetchContent_Declare(libjxl
  GIT_REPOSITORY  https://github.com/libjxl/libjxl
  GIT_TAG         c8b0bf3789351c9ccd8c48441354c522fd9ab188
  BINARY_DIR      build
  SUBBUILD_DIR    subbuild
)

set(JPEGXL_ENABLE_MANPAGES OFF CACHE INTERNAL "Build and install man pages for the command-line tools.")
set(JPEGXL_ENABLE_BENCHMARK OFF CACHE INTERNAL "Build JPEGXL benchmark tools.")
set(JPEGXL_ENABLE_EXAMPLES OFF CACHE INTERNAL "Build JPEGXL library usage examples.")
set(JPEGXL_ENABLE_SJPEG OFF CACHE INTERNAL "Build JPEGXL with support for encoding with sjpeg.")
set(JPEGXL_ENABLE_OPENEXR OFF CACHE INTERNAL "Build JPEGXL with support for OpenEXR if available.")

option(BUILD_TESTING "" OFF)

FetchContent_MakeAvailable(libjxl)

target_include_directories(imagedecoder PRIVATE ${libjxl_BINARY_DIR} ${libjxl_SOURCE_DIR})
target_link_libraries(imagedecoder jxl_dec-static jxl_threads-static)
