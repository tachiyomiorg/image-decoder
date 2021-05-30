//
// Created by len on 30/5/21.
//

#ifndef IMAGEDECODER_DECODER_HEADERS_H
#define IMAGEDECODER_DECODER_HEADERS_H

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

static const uint8_t heifMagic[] =
  {'f', 't', 'y', 'p'};

static const uint8_t heifBrands[][4] = {
  {'h', 'e', 'i', 'c'},
  {'h', 'e', 'i', 'x'},
  {'h', 'e', 'v', 'c'},
  {'h', 'e', 'i', 'm'},
  {'h', 'e', 'i', 's'},
  {'h', 'e', 'v', 'm'},
  {'h', 'e', 'v', 's'},
  {'m', 'i', 'f', '1'},
  {'m', 's', 'f', '1'}
};

bool is_heif(const uint8_t* data) {
  if (memcmp(data + 4, heifMagic, 4) != 0) {
    return false;
  }

  for (uint32_t i = 0; i < sizeof(heifBrands); i++) {
    if (memcmp(data + 8, heifBrands[i], 4) == 0) {
      return true;
    }
  }
  return false;
}

#endif //IMAGEDECODER_DECODER_HEADERS_H
