//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_DECODER_JPEG_H
#define IMAGEDECODER_DECODER_JPEG_H

#include <stdio.h>
#include <memory>
#include "decoder_base.h"
#include "jpeglib.h"
#include "log.h"

// Wrap the JPEG C API in this class to automatically manage memory
class JpegDecodeSession {
public:
  JpegDecodeSession();
  ~JpegDecodeSession();

  jpeg_decompress_struct jinfo;
  jpeg_error_mgr jerr;

  void init(Stream* stream);
};

class JpegDecoder: public BaseDecoder {
public:
  JpegDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders);
  static bool handles(const uint8_t* stream);

  void decode(uint8_t *outPixels, Rect outRect, Rect srcRegion, bool rgb565,
              uint32_t sampleSize);

private:
  ImageInfo parseInfo();
  std::unique_ptr<JpegDecodeSession> initDecodeSession();
};

#endif //IMAGEDECODER_DECODER_JPEG_H
