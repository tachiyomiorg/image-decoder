FetchContent_Declare(lcms2
  GIT_REPOSITORY  https://github.com/mm2/Little-CMS
  GIT_TAG         lcms2.12
  PATCH_COMMAND   git apply ${CMAKE_CURRENT_LIST_DIR}/lcms2.12.patch
  BINARY_DIR      build
  SUBBUILD_DIR    subbuild
)

FetchContent_MakeAvailable(lcms2)

target_include_directories(imagedecoder PRIVATE ${lcms2_BINARY_DIR} ${lcms2_SOURCE_DIR})
target_link_libraries(imagedecoder lcms2)
