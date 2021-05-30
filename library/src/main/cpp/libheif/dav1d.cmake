include(FetchContent)

FetchContent_Declare(dav1d
  GIT_REPOSITORY  https://github.com/videolan/dav1d.git
  GIT_TAG         0.9.0
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_MakeAvailable(dav1d)

set(DAV1D_FILENAME "${dav1d_BINARY_DIR}/src/libdav1d.a")

if (NOT EXISTS "${DAV1D_FILENAME}")
  set(PREPARE_DAV1D "ANDROID_NDK=${CMAKE_ANDROID_NDK} ${CMAKE_CURRENT_LIST_DIR}/generate_dav1d_android_cross_compile.sh -a ${CMAKE_ANDROID_ARCH}")
  set(CONFIG_DAV1D "meson --buildtype release --default-library static --cross-file android_cross.txt -Denable_tools=false -Denable_tests=false -Dbitdepths=8 ${dav1d_SOURCE_DIR}")
  set(BUILD_DAV1D "ninja")

  execute_process(
    COMMAND bash -c "${PREPARE_DAV1D} && ${CONFIG_DAV1D} && ${BUILD_DAV1D}"
    WORKING_DIRECTORY ${dav1d_BINARY_DIR}
  )
endif()

if(NOT EXISTS "${DAV1D_FILENAME}")
  message(FATAL_ERROR "libavif: ${DAV1D_FILENAME} is missing")
endif()

set(DAV1D_FOUND 1)
set(DAV1D_LIBRARIES "${DAV1D_FILENAME}")
set(DAV1D_INCLUDE_DIR
  "${dav1d_BINARY_DIR}"
  "${dav1d_BINARY_DIR}/include"
  "${dav1d_BINARY_DIR}/include/dav1d"
  "${dav1d_SOURCE_DIR}/include"
)
