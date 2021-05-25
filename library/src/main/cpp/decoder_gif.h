//
// Created by len on 25/5/21.
//

#ifndef IMAGEDECODER_DECODER_GIF_H
#define IMAGEDECODER_DECODER_GIF_H

#include <sys/types.h>

// libgif is not included for now, so this decoder can only parse the magic numbers.
class GifDecoder {
public:
  static bool handles(const uint8_t* stream) {
    return stream[0] == 0xFF && stream[1] == 0xD8 && stream[2] == 0xFF;
  }
};

#endif //IMAGEDECODER_DECODER_GIF_H
