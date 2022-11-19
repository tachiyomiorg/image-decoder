//
// Created by len on 29/12/20.
//

#ifndef IMAGEDECODER_RECT_H
#define IMAGEDECODER_RECT_H

#include <sys/types.h>

struct Rect {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;

  Rect downsample(uint32_t scale) {
    if (scale == 1) {
      return *this;
    }
    return {
        .x = x / scale,
        .y = y / scale,
        .width = width / scale,
        .height = height / scale,
    };
  }

  Rect upsample(uint32_t scale) {
    if (scale == 1) {
      return *this;
    }
    return {
        .x = x * scale,
        .y = y * scale,
        .width = width * scale,
        .height = height * scale,
    };
  }
};

#endif // IMAGEDECODER_RECT_H
