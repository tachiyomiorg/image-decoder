//
// Created by len on 30/12/20.
//

#ifndef IMAGEDECODER_DECODER_WEBP_H
#define IMAGEDECODER_DECODER_WEBP_H

#include "decoder_base.h"
#include <src/webp/decode.h>
#include <src/webp/demux.h>

class WebpDecoder : public BaseDecoder {
public:
  WebpDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders,
              cmsHPROFILE targetProfile);

  void decode(uint8_t* outPixels, Rect outRect, Rect inRect,
              uint32_t sampleSize);

private:
  ImageInfo parseInfo();
  cmsHPROFILE getColorProfile();
};

#endif // IMAGEDECODER_DECODER_WEBP_H
