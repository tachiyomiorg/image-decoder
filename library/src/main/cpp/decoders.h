//
// Created by len on 30/5/21.
//

#ifndef IMAGEDECODER_DECODERS_H
#define IMAGEDECODER_DECODERS_H

#include "decoder_headers.h"
#ifdef HAVE_LIBJPEG
#include "decoder_jpeg.h"
#endif
#ifdef HAVE_LIBPNG
#include "decoder_png.h"
#endif
#ifdef HAVE_LIBWEBP
#include "decoder_webp.h"
#endif
#ifdef HAVE_LIBHEIF
#include "decoder_heif.h"
#endif
#ifdef HAVE_LIBJXL
#include "decoder_jxl.h"
#endif

#endif // IMAGEDECODER_DECODERS_H
