set(JPEGXL_VERSION "v0.9.2" CACHE INTERNAL "")
FetchContent_Declare(libjxl
  GIT_REPOSITORY https://github.com/libjxl/libjxl
  GIT_TAG ${JPEGXL_VERSION}
  BINARY_DIR build
  SUBBUILD_DIR subbuild
)

option(BUILD_TESTING "" OFF)
set(JPEGXL_ENABLE_DOXYGEN OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_MANPAGES OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_EXAMPLES OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_SJPEG OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_OPENEXR OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_TRANSCODE_JPEG OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_TOOLS OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_BENCHMARK OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_JPEGLI OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_JNI OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_DEVTOOLS OFF CACHE INTERNAL "")
set(JPEGXL_ENABLE_BENCHMARK OFF CACHE INTERNAL "")

FetchContent_GetProperties(libjxl)

if(NOT libjxl_POPULATED)
  FetchContent_Populate(libjxl)
  add_subdirectory(${libjxl_SOURCE_DIR} ${libjxl_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

target_include_directories(imagedecoder PRIVATE
  ${libjxl_BINARY_DIR}
  ${libjxl_SOURCE_DIR}
  $<TARGET_PROPERTY:hwy,INCLUDE_DIRECTORIES>
)

target_link_libraries(imagedecoder
  jxl_dec
  jxl_threads
  brotlidec
  brotlienc
  brotlicommon
  hwy
  -Wl,--allow-multiple-definition
)
