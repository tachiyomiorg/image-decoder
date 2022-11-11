FetchContent_Declare(dav1d
  GIT_REPOSITORY    https://github.com/videolan/dav1d.git
  GIT_TAG           1.0.0
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  BINARY_DIR        dav1d-build
  SUBBUILD_DIR      dav1d-subbuild
)
FetchContent_MakeAvailable(dav1d)

set(DAV1D_FILENAME "${dav1d_BINARY_DIR}/src/libdav1d.a")

if(NOT EXISTS "${DAV1D_FILENAME}")
  set(ENV{ANDROID_NDK} ${CMAKE_ANDROID_NDK})
  set(PREPARE_DAV1D "${CMAKE_CURRENT_LIST_DIR}/generate_dav1d_android_cross_compile.sh -a ${CMAKE_ANDROID_ARCH}")
  set(CONFIG_DAV1D "meson --buildtype release --default-library static --cross-file android_cross.txt -Denable_tools=false -Denable_tests=false -Dbitdepths=8 ${dav1d_SOURCE_DIR}")
  set(BUILD_DAV1D "ninja")

  if(DEFINED ENV{JITPACK})
    # Why don't they have python 3 :(
    FetchContent_Declare(python
      URL https://github.com/kageiit/jitpack-python/releases/download/3.8/python-3.8-ubuntu-16.tar.gz
      BINARY_DIR        python-build
      SUBBUILD_DIR      python-subbuild
    )
    FetchContent_MakeAvailable(python)
    set(ENV{PATH} "${python_SOURCE_DIR}/bin:$ENV{PATH}")

    FetchContent_Declare(meson
      URL https://github.com/mesonbuild/meson/releases/download/0.58.0/meson-0.58.0.tar.gz
      BINARY_DIR        meson-build
      SUBBUILD_DIR      meson-subbuild
    )
    FetchContent_MakeAvailable(meson)
    file(RENAME ${meson_SOURCE_DIR}/meson.py ${meson_SOURCE_DIR}/meson)
    get_filename_component(ninja_PATH ${CMAKE_COMMAND} DIRECTORY)
    set(ENV{PATH} "${meson_SOURCE_DIR}:${ninja_PATH}:$ENV{PATH}")

    FetchContent_Declare(nasm
      URL http://mirrors.kernel.org/ubuntu/pool/universe/n/nasm/nasm_2.14.02-1_amd64.deb
      BINARY_DIR        nasm-build
      SUBBUILD_DIR      nasm-subbuild
      PATCH_COMMAND     bash -c "tar xvf data.tar.xz || true"
    )
    FetchContent_MakeAvailable(nasm)
    set(ENV{PATH} "${nasm_SOURCE_DIR}/usr/bin:$ENV{PATH}")
  endif()

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
