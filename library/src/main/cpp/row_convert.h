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

#ifndef IMAGE_IMAGE_CONVERT_H
#define IMAGE_IMAGE_CONVERT_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

void RGBA8888_to_RGBA8888_row(uint8_t* dst,
    const uint8_t* src1, const uint8_t* src2,
    uint32_t d_width, uint32_t ratio);

void RGBA8888_to_RGB565_row(uint8_t* dst,
    const uint8_t* src1, const uint8_t* src2,
    uint32_t d_width, uint32_t ratio);

#endif //IMAGE_IMAGE_CONVERT_H
