//
// Created by w on 02/07/21.
//

#ifndef IMAGEDECODER_DECODER_JXL_H
#define IMAGEDECODER_DECODER_JXL_H

#include "decoder_base.h"
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner.h>
#include <jxl/resizable_parallel_runner_cxx.h>

class JpegxlDecoder : public BaseDecoder {
public:
  JpegxlDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders);

  void decode(uint8_t* outPixels, Rect outRect, Rect inRect,
              uint32_t sampleSize, cmsHPROFILE targetProfile);

private:
  void decode();

  ImageInfo parseInfo();
  std::vector<uint8_t> pixels;
  JxlBasicInfo jxl_info;
  cmsHPROFILE mSrcProfile;
  bool transformed = false;
};

#endif // IMAGEDECODER_DECODER_JXL_H
