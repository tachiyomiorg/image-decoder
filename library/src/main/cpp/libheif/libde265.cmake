FetchContent_Declare(libde265
  GIT_REPOSITORY  https://github.com/strukturag/libde265.git
  GIT_TAG         v1.0.8
  BINARY_DIR      libde265-build
  SUBBUILD_DIR    libde265-subbuild
)

option(BUILD_SHARED_LIBS "" OFF)
option(ENABLE_SDL "" OFF)
if (${ANDROID} AND NOT ${ANDROID_ABI} STREQUAL "x86" AND NOT ${ANDROID_ABI} STREQUAL "x86_64")
  option(DISABLE_SSE "" ON)
endif()

FetchContent_MakeAvailable(libde265)

set(LIBDE265_FOUND 1)
set(LIBDE265_LIBRARIES libde265)
set(LIBDE265_INCLUDE_DIR
  "${libde265_SOURCE_DIR}"
  "${libde265_BINARY_DIR}"
)
