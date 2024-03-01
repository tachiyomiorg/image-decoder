//
// Created by len on 27/5/21.
//

#ifndef IMAGEDECODER_DECODER_HEIF_H
#define IMAGEDECODER_DECODER_HEIF_H

#include "decoder_base.h"
#include <libheif/heif.h>
#include <libheif/heif_cxx.h>

bool is_libheif_compatible(const uint8_t* bytes, uint32_t size);

class HeifDecoder : public BaseDecoder {
public:
  HeifDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders,
              cmsHPROFILE targetProfile);

  void decode(uint8_t* outPixels, Rect outRect, Rect inRect,
              uint32_t sampleSize);

private:
  ImageInfo parseInfo();
  cmsHPROFILE getColorProfile(heif::ImageHandle handle);
};

#endif // IMAGEDECODER_DECODER_HEIF_H
