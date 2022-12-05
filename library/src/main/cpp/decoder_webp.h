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
  WebpDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders);

  void decode(uint8_t* outPixels, Rect outRect, Rect inRect, bool rgb565,
              uint32_t sampleSize, cmsHPROFILE targetProfile);

private:
  ImageInfo parseInfo();
  cmsHPROFILE getColorProfile();
};

#endif // IMAGEDECODER_DECODER_WEBP_H
