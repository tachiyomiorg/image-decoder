//
// Created by len on 27/5/21.
//

#ifndef IMAGEDECODER_DECODER_HEIF_H
#define IMAGEDECODER_DECODER_HEIF_H

#include "decoder_base.h"

class HeifDecoder: public BaseDecoder {
public:
    HeifDecoder(std::unique_ptr<Stream>&& stream, bool cropBorders);
    static bool handles(const uint8_t* stream);

    void decode(uint8_t* outPixels, Rect outRect, Rect inRect, bool rgb565, uint32_t sampleSize);

private:
    ImageInfo parseInfo();
};

#endif //IMAGEDECODER_DECODER_HEIF_H
