//
// Created by len on 26/12/20.
//

#ifndef IMAGEDECODER_STREAM_H
#define IMAGEDECODER_STREAM_H

struct Stream {
    uint8_t* bytes;
    uint32_t size;

    Stream(uint8_t* bytes, uint32_t size) : bytes(bytes), size(size) {}
};

#endif //IMAGEDECODER_STREAM_H
