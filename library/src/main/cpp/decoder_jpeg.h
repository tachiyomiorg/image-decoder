//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_DECODER_JPEG_H
#define IMAGEDECODER_DECODER_JPEG_H

#include "decoder_base.h"
#include "jpeglib.h"
#include "log.h"
#include <memory>
#include <stdio.h>

// Wrap the JPEG C API in this class to automatically manage memory
class JpegDecodeSession {
public:
  JpegDecodeSession();
  ~JpegDecodeSession();

  jpeg_decompress_struct jinfo;
  jpeg_error_mgr jerr;

  void init(Stream* stream);
};

class JpegDecoder : public BaseDecoder {
public:
  JpegDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders);

  void decode(uint8_t* outPixels, Rect outRect, Rect srcRegion,
              uint32_t sampleSize, cmsHPROFILE targetProfile);

private:
  ImageInfo parseInfo();
  std::unique_ptr<JpegDecodeSession> initDecodeSession();
  cmsHPROFILE getColorProfile(jpeg_decompress_struct* jinfo);
};

#endif // IMAGEDECODER_DECODER_JPEG_H
