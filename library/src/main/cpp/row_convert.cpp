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

void RGBA8888_to_RGBA8888_row_internal_2(uint8_t* dst, const uint8_t* src1,
                                         const uint8_t* src2, uint32_t d_width,
                                         uint32_t ratio) {
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

    dst[0] = (uint8_t)(r / 4);
    dst[1] = (uint8_t)(g / 4);
    dst[2] = (uint8_t)(b / 4);
    dst[3] = (uint8_t)(a / 4);

    src1 += interval;
    src2 += interval;
    dst += 4;
  }
}

void RGBA8888_to_RGBA8888_row(uint8_t* dst, const uint8_t* src1,
                              const uint8_t* src2, uint32_t d_width,
                              uint32_t ratio) {
  if (ratio == 1) {
    memcpy(dst, src1, d_width * 4);
  } else {
    RGBA8888_to_RGBA8888_row_internal_2(dst, src1, src2, d_width, ratio);
  }
}

#pragma clang diagnostic pop
