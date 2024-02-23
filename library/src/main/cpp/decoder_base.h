//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_DECODER_BASE_H
#define IMAGEDECODER_DECODER_BASE_H

#include "borders.h"
#include "java_stream.h"
#include <include/lcms2.h>
#include <vector>

struct ImageInfo {
  uint32_t imageWidth;
  uint32_t imageHeight;
  bool isAnimated;
  Rect bounds;
};

class BaseDecoder {
public:
  BaseDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders,
              cmsHPROFILE targetProfile) {
    this->stream = std::move(stream);
    this->cropBorders = cropBorders;
    this->targetProfile = targetProfile;
  }
  virtual ~BaseDecoder() {
    if (transform) {
      cmsDeleteTransform(transform);
    }
    if (targetProfile) {
      cmsCloseProfile(targetProfile);
    }
  };

  virtual void decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                      uint32_t sampleSize) = 0;

protected:
  std::shared_ptr<Stream> stream;

public:
  bool cropBorders;
  cmsHPROFILE targetProfile;
  ImageInfo info;
  cmsHTRANSFORM transform = nullptr;
  bool useTransform = false;
  cmsUInt32Number inType;
};

#endif // IMAGEDECODER_DECODER_BASE_H
