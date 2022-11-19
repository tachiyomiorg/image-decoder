//
// Created by len on 25/12/20.
//

#ifndef IMAGEDECODER_BORDERS_H
#define IMAGEDECODER_BORDERS_H

#include "rect.h"

/** A line will be considered as having content if 0.25% of it is filled. */
const float filledRatioLimit = 0.0025;

/** When the threshold is closer to 1, less content will be cropped. **/
#define THRESHOLD 0.75

const uint8_t thresholdForBlack = (uint8_t)(255.0 * THRESHOLD);

const uint8_t thresholdForWhite = (uint8_t)(255.0 - 255.0 * THRESHOLD);

/** Finds the borders of the image. This only works on bitmaps of a single
 * component (grayscale) **/
Rect findBorders(uint8_t* pixels, uint32_t width, uint32_t height);

#endif // IMAGEDECODER_BORDERS_H
