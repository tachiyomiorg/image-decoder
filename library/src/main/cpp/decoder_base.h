//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_DECODER_BASE_H
#define IMAGEDECODER_DECODER_BASE_H

#include "java_stream.h"
#include "borders.h"

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
  virtual ~BaseDecoder() {};

  virtual void decode(uint8_t* outPixels, Rect outRect, Rect inRect, bool rgb565, uint32_t sampleSize) = 0;

protected:
  std::shared_ptr<Stream> stream;

public:
  bool cropBorders;
  ImageInfo info;
};

#endif //IMAGEDECODER_DECODER_BASE_H
