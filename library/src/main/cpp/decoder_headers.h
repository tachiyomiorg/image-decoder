//
// Created by len on 30/5/21.
//

#ifndef IMAGEDECODER_DECODER_HEADERS_H
#define IMAGEDECODER_DECODER_HEADERS_H

#include <algorithm>

bool is_jpeg(uint8_t* data) {
  return data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
}

bool is_png(uint8_t* data) {
  return data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G';
}

bool is_webp(const uint8_t* data) {
  return data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F';
}

bool is_gif(const uint8_t* data) {
  return data[0] == 'G' && data[1] == 'I' && data[2] == 'F' && data[3] == '8';
}

enum ftyp_image_type {
  ftyp_image_type_no,
  ftyp_image_type_heif,
  ftyp_image_type_avif
};

ftyp_image_type get_ftyp_image_type(const uint8_t* data, uint32_t size) {
  if (data[4] != 'f' || data[5] != 't' || data[6] != 'y' || data[7] != 'p') {
    return ftyp_image_type_no;
  }

  uint32_t headerSize = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
  uint32_t maxOffset = std::min(headerSize, size) - 4;
  uint32_t offset = 8;
  while (offset <= maxOffset) {
    auto brand = data + offset;
    if (brand[0] == 'h' && brand[1] == 'e' &&
        (brand[2] == 'i' || brand[2] == 'v')) {
      return ftyp_image_type_heif;
    } else if (brand[0] == 'a' && brand[1] == 'v' && brand[2] == 'i') {
      return ftyp_image_type_avif;
    }
    offset += 4;
  }

  return ftyp_image_type_no;
}

bool is_jxl(const uint8_t* data) {
  return (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0xC &&
          data[4] == 'J' && data[5] == 'X' && data[6] == 'L' &&
          data[7] == ' ' && data[8] == 0xD && data[9] == 0xA &&
          data[10] == 0x87 && data[11] == 0xA) || // container
         (data[0] == 0xff && data[1] == 0x0a);    // codestream
}

#endif // IMAGEDECODER_DECODER_HEADERS_H
