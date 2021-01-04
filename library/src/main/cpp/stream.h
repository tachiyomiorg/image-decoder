//
// Created by len on 26/12/20.
//

#ifndef IMAGEDECODER_STREAM_H
#define IMAGEDECODER_STREAM_H

#include "log.h"

class Stream {
public:
  Stream(uint8_t* bytes, uint32_t size) : bytes(bytes), size(size) {}
  ~Stream() {
    free(bytes);
  }
  Stream & operator=(const Stream&) = delete;
  Stream(const Stream&) = delete;
  Stream() = default;

  uint8_t* bytes;
  uint32_t size;
};

#endif //IMAGEDECODER_STREAM_H
