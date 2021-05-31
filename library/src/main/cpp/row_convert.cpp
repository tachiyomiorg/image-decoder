/*
 * Copyright 2016 Hippo Seven
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include "row_convert.h"

#define RGB565_BLUE(c) ((c) & 0x1f)
#define RGB565_GREEN(c1, c2) ((((c1) & 0xe0) >> 5) | (((c2) & 0x7) << 3))
#define RGB565_REG(c) ((c) >> 3)

void RGBA8888_to_RGBA8888_row_internal_2(
  uint8_t* dst, const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  uint32_t i;
  uint32_t start = (ratio - 2) / 2 * 4;
  uint32_t interval = ratio * 4;

  src1 += start;
  src2 += start;
  for (i = 0; i < d_width; i++) {
    uint16_t r, g, b, a;
    r = src1[0] + src2[0];
    g = src1[1] + src2[1];
    b = src1[2] + src2[2];
    a = src1[3] + src2[3];
    r += src1[4] + src2[4];
    g += src1[5] + src2[5];
    b += src1[6] + src2[6];
    a += src1[7] + src2[7];

    dst[0] = (uint8_t) (r / 4);
    dst[1] = (uint8_t) (g / 4);
    dst[2] = (uint8_t) (b / 4);
    dst[3] = (uint8_t) (a / 4);

    src1 += interval;
    src2 += interval;
    dst += 4;
  }
}

void RGBA8888_to_RGBA8888_row(uint8_t* dst,
  const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  if (ratio == 1) {
    memcpy(dst, src1, d_width * 4);
  } else {
    RGBA8888_to_RGBA8888_row_internal_2(dst, src1, src2, d_width, ratio);
  }
}

static void RGBA8888_to_RGB565_row_internal_1(uint8_t* dst, const uint8_t* src, uint32_t width) {
  uint32_t i;
  for (i = 0; i < width; i++) {
    uint8_t r, g, b;
    r = src[0] >> 3;
    g = src[1] >> 2;
    b = src[2] >> 3;

    dst[0] = (uint8_t) (g << 5 | b);
    dst[1] = (uint8_t) (r << 3 | g >> 3);

    src += 4;
    dst += 2;
  }
}

static void RGBA8888_to_RGB565_row_internal_2(
  uint8_t* dst, const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  uint32_t i;
  uint32_t start = (ratio - 2) / 2 * 4;
  uint32_t interval = ratio * 4;

  src1 += start;
  src2 += start;
  for (i = 0; i < d_width; i++) {
    uint16_t r, g, b;
    r = src1[0] + src2[0];
    g = src1[1] + src2[1];
    b = src1[2] + src2[2];
    r += src1[4] + src2[4];
    g += src1[5] + src2[5];
    b += src1[6] + src2[6];
    r = (uint16_t) ((r / 4) >> 3);
    g = (uint16_t) ((g / 4) >> 2);
    b = (uint16_t) ((b / 4) >> 3);

    dst[0] = (uint8_t) (g << 5 | b);
    dst[1] = (uint8_t) (r << 3 | g >> 3);

    src1 += interval;
    src2 += interval;
    dst += 2;
  }
}

void RGBA8888_to_RGB565_row(uint8_t* dst,
  const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  if (ratio == 1) {
    RGBA8888_to_RGB565_row_internal_1(dst, src1, d_width);
  } else {
    RGBA8888_to_RGB565_row_internal_2(dst, src1, src2, d_width, ratio);
  }
}


static void RGB565_to_RGB565_row_internal_2(
  uint8_t* dst, const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  uint32_t i;
  uint32_t start = (ratio - 2) / 2 * 2;
  uint32_t interval = ratio * 2;

  src1 += start;
  src2 += start;
  for (i = 0; i < d_width; i++) {
    uint8_t r, g, b;

    b = (uint8_t) RGB565_BLUE(src1[0]) + (uint8_t) RGB565_BLUE(src2[0]);
    g = (uint8_t) RGB565_GREEN(src1[0], src1[1]) + (uint8_t) RGB565_GREEN(src2[0], src2[1]);
    r = RGB565_REG(src1[1]) + RGB565_REG(src2[1]);
    b += (uint8_t) RGB565_BLUE(src1[2]) + (uint8_t) RGB565_BLUE(src2[2]);
    g += (uint8_t) RGB565_GREEN(src1[2], src1[3]) + (uint8_t) RGB565_GREEN(src2[2], src2[3]);
    r += RGB565_REG(src1[3]) + RGB565_REG(src2[3]);

    b /= 4;
    g /= 4;
    r /= 4;

    dst[0] = b | g << 5;
    dst[1] = g >> 3 | r << 3;

    src1 += interval;
    src2 += interval;
    dst += 2;
  }
}

void RGB565_to_RGB565_row(uint8_t* dst,
  const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  if (ratio == 1) {
    memcpy(dst, src1, d_width * 2);
  } else {
    RGB565_to_RGB565_row_internal_2(dst, src1, src2, d_width, ratio);
  }
}

static void RGB888_to_RGB565_row_internal_1(uint8_t* dst, const uint8_t* src, uint32_t width) {
  uint32_t i;
  for (i = 0; i < width; i++) {
    uint8_t r, g, b;
    r = src[0] >> 3;
    g = src[1] >> 2;
    b = src[2] >> 3;

    dst[0] = (uint8_t) (g << 5 | b);
    dst[1] = (uint8_t) (r << 3 | g >> 3);

    src += 3;
    dst += 2;
  }
}

static void RGB888_to_RGB565_row_internal_2(
  uint8_t* dst, const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  uint32_t i;
  uint32_t start = (ratio - 2) / 2 * 3;
  uint32_t interval = ratio * 3;

  src1 += start;
  src2 += start;
  for (i = 0; i < d_width; i++) {
    uint16_t r, g, b;
    r = src1[0] + src2[0];
    g = src1[1] + src2[1];
    b = src1[2] + src2[2];
    r += src1[3] + src2[3];
    g += src1[4] + src2[4];
    b += src1[5] + src2[5];
    r = (uint16_t) ((r / 4) >> 3);
    g = (uint16_t) ((g / 4) >> 2);
    b = (uint16_t) ((b / 4) >> 3);

    dst[0] = (uint8_t) (g << 5 | b);
    dst[1] = (uint8_t) (r << 3 | g >> 3);

    src1 += interval;
    src2 += interval;
    dst += 2;
  }
}

void RGB888_to_RGB565_row(uint8_t* dst,
  const uint8_t* src1, const uint8_t* src2,
  uint32_t d_width, uint32_t ratio
) {
  if (ratio == 1) {
    RGB888_to_RGB565_row_internal_1(dst, src1, d_width);
  } else {
    RGB888_to_RGB565_row_internal_2(dst, src1, src2, d_width, ratio);
  }
}

#pragma clang diagnostic pop
