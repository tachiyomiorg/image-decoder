//
// Created by len on 24/12/20.
//

#ifndef IMAGEDECODER_DECODER_PNG_H
#define IMAGEDECODER_DECODER_PNG_H

#include "decoder_base.h"
#include "png.h"

struct PngReader {
  uint8_t* bytes;
  uint32_t read;
  uint32_t remain;
};

// Wrap the PNG C API in this class to automatically manage memory
class PngDecodeSession {
public:
  PngDecodeSession(Stream* stream);
  ~PngDecodeSession();

  png_struct* png;
  png_info* pinfo;
  PngReader reader;

  void init();
};

class PngDecoder : public BaseDecoder {
public:
  PngDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders);

  void decode(uint8_t* outPixels, Rect outRect, Rect inRect, bool rgb565,
              uint32_t sampleSize, cmsHPROFILE targetProfile);

private:
  ImageInfo parseInfo();
  std::unique_ptr<PngDecodeSession> initDecodeSession();
  cmsHPROFILE getColorProfile(png_struct* png, png_info* pinfo,
                              uint8_t colorType);
};

#endif // IMAGEDECODER_DECODER_PNG_H
