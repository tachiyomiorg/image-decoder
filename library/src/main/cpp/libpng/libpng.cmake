include(FetchContent)

FetchContent_Declare(libpng
  GIT_REPOSITORY  https://github.com/glennrp/libpng
  GIT_TAG         v1.6.37
)

option(PNG_SHARED "" OFF)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)

FetchContent_MakeAvailable(libpng)

include_directories(${libpng_BINARY_DIR} ${libpng_SOURCE_DIR})
target_link_libraries(imagedecoder png_static z)
