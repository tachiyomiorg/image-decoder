//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_DECODER_BASE_H
#define IMAGEDECODER_DECODER_BASE_H

#include "borders.h"
#include "java_stream.h"
#include <lcms2.h>
#include <vector>

struct ImageInfo {
  uint32_t imageWidth;
  uint32_t imageHeight;
  bool isAnimated;
  Rect bounds;
};

class BaseDecoder {
public:
  BaseDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders) {
    this->stream = std::move(stream);
    this->cropBorders = cropBorders;
  }
  virtual ~BaseDecoder() {
    if (transform) {
      cmsDeleteTransform(transform);
    }
  };

  virtual void decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                      bool rgb565, uint32_t sampleSize,
                      cmsHPROFILE targetProfile) = 0;

protected:
  std::shared_ptr<Stream> stream;

public:
  bool cropBorders;
  ImageInfo info;
  cmsHTRANSFORM transform = nullptr;
  bool useTransform = false;
};

#endif // IMAGEDECODER_DECODER_BASE_H
