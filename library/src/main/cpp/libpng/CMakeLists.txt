FetchContent_Declare(libpng
  GIT_REPOSITORY  https://github.com/glennrp/libpng
  GIT_TAG         v1.6.40
  BINARY_DIR      build
  SUBBUILD_DIR    subbuild
)

option(PNG_SHARED "" OFF)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)

FetchContent_MakeAvailable(libpng)

target_include_directories(imagedecoder PRIVATE ${libpng_BINARY_DIR} ${libpng_SOURCE_DIR})
target_link_libraries(imagedecoder png_static z)
