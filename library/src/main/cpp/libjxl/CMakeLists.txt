FetchContent_Declare(libjxl
  GIT_REPOSITORY https://github.com/libjxl/libjxl
  GIT_TAG v0.10.0
  BINARY_DIR build
  SUBBUILD_DIR subbuild
)

option(BUILD_TESTING "" OFF)
option(JPEGXL_ENABLE_DOXYGEN "" OFF)
option(JPEGXL_ENABLE_MANPAGES "" OFF)
option(JPEGXL_ENABLE_EXAMPLES "" OFF)
option(JPEGXL_ENABLE_SJPEG "" OFF)
option(JPEGXL_ENABLE_OPENEXR "" OFF)
option(JPEGXL_ENABLE_TRANSCODE_JPEG "" OFF)
option(JPEGXL_ENABLE_TOOLS "" OFF)
option(JPEGXL_ENABLE_BENCHMARK "" OFF)
option(JPEGXL_ENABLE_JPEGLI "" OFF)
option(JPEGXL_ENABLE_JNI "" OFF)
option(JPEGXL_ENABLE_DEVTOOLS "" OFF)
option(JPEGXL_ENABLE_BENCHMARK "" OFF)

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
