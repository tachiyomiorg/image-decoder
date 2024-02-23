//
// Created by len on 24/12/20.
//

#include "decoder_png.h"
#include "row_convert.h"
#include <algorithm>

static void png_skip_rows(png_structrp png_ptr, png_uint_32 num_rows) {
  for (png_uint_32 i = 0; i < num_rows; ++i) {
    png_read_row(png_ptr, nullptr, nullptr);
  }
}

PngDecoder::PngDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders,
                       cmsHPROFILE targetProfile)
    : BaseDecoder(std::move(stream), cropBorders, targetProfile) {
  this->info = parseInfo();
}

PngDecodeSession::PngDecodeSession(Stream* stream)
    : png(nullptr), pinfo(nullptr),
      reader({.bytes = stream->bytes, .read = 0, .remain = stream->size}) {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
void PngDecodeSession::init() {
  auto errorFn = [](png_struct*, png_const_charp msg) {
    throw std::runtime_error(msg);
  };
  auto warnFn = [](png_struct*, png_const_charp msg) { LOGW("%s", msg); };
  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, errorFn, warnFn);
  if (!png) {
    throw std::runtime_error("Failed to create png read struct");
  }
  pinfo = png_create_info_struct(png);
  if (!pinfo) {
    throw std::runtime_error("Failed to create png info struct");
  }

  auto readFn = [](png_struct* p, png_byte* data, png_size_t length) {
    auto* r = (PngReader*)png_get_io_ptr(p);
    uint32_t next = std::min(r->remain, (uint32_t)length);
    if (next > 0) {
      memcpy(data, r->bytes + r->read, next);
      r->read += next;
      r->remain -= next;
    }
  };
  png_set_read_fn(png, &reader, readFn);
  png_read_info(png, pinfo);
}
#pragma clang diagnostic pop

PngDecodeSession::~PngDecodeSession() {
  png_destroy_read_struct(&png, &pinfo, nullptr);
}

std::unique_ptr<PngDecodeSession> PngDecoder::initDecodeSession() {
  auto session = std::make_unique<PngDecodeSession>(stream.get());
  session->init();
  return session;
}

ImageInfo PngDecoder::parseInfo() {
  auto session = initDecodeSession();
  auto png = session->png;
  auto pinfo = session->pinfo;

  uint32_t imageWidth = png_get_image_width(png, pinfo);
  uint32_t imageHeight = png_get_image_height(png, pinfo);

  Rect bounds = {.x = 0, .y = 0, .width = imageWidth, .height = imageHeight};
  if (cropBorders) {
    try {
      auto pixels = std::make_unique<uint8_t[]>(imageWidth * imageHeight);

      uint8_t colorType = png_get_color_type(png, pinfo);
      uint8_t bitDepth = png_get_bit_depth(png, pinfo);

      png_set_expand(png);
      if (bitDepth == 16) {
        png_set_scale_16(png);
      }
      if (colorType & (uint8_t)PNG_COLOR_MASK_COLOR) {
        png_set_rgb_to_gray(png, 1, -1, -1);
        png_set_strip_alpha(png);
      } else if (colorType & (uint8_t)PNG_COLOR_MASK_ALPHA) {
        png_set_strip_alpha(png);
      }

      int32_t passes = png_set_interlace_handling(png);

      uint8_t* pixelsPos;
      while (--passes >= 0) {
        pixelsPos = pixels.get();
        for (uint32_t i = 0; i < imageHeight; ++i) {
          png_read_row(png, pixelsPos, nullptr);
          pixelsPos += imageWidth;
        }
      }
      bounds = findBorders(pixels.get(), imageWidth, imageHeight);
    } catch (std::bad_alloc& ex) {
      LOGW("Couldn't crop borders on a PNG image of size %dx%d", imageWidth,
           imageHeight);
    }
  }

  return ImageInfo{
      .imageWidth = imageWidth,
      .imageHeight = imageHeight,
      .isAnimated = false,
      .bounds = bounds,
  };
}

cmsHPROFILE PngDecoder::getColorProfile(png_struct* png, png_info* pinfo,
                                        uint8_t colorType) {
  if (!png_get_valid(png, pinfo, PNG_INFO_iCCP)) {
    return nullptr;
  }

  png_charp name;
  png_bytep icc_data;
  png_uint_32 icc_size;
  int comp_type;
  png_get_iCCP(png, pinfo, &name, &comp_type, &icc_data, &icc_size);

  cmsHPROFILE src_profile = cmsOpenProfileFromMem(icc_data, icc_size);
  cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);

  bool rgb = colorType & PNG_COLOR_MASK_COLOR;

  if (rgb && profileSpace != cmsSigRgbData ||
      !rgb && profileSpace != cmsSigGrayData) {
    cmsCloseProfile(src_profile);
    return nullptr;
  }

  return src_profile;
}

void PngDecoder::decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                        uint32_t sampleSize) {
  auto session = initDecodeSession();
  auto png = session->png;
  auto pinfo = session->pinfo;

  uint8_t colorType = png_get_color_type(png, pinfo);
  uint8_t bitDepth = png_get_bit_depth(png, pinfo);

  png_set_expand(png);

  if (bitDepth == 16) {
    png_set_scale_16(png);
  }

  cmsHPROFILE src_profile = getColorProfile(png, pinfo, colorType);
  if (!src_profile) {
    src_profile = cmsCreate_sRGBProfile();
    inType = TYPE_RGBA_8;

    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
      png_set_gray_to_rgb(png);
    }
  } else {
    if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA ||
        colorType == PNG_COLOR_TYPE_GRAY) {
      inType = TYPE_GRAYA_8;
    } else {
      inType = TYPE_RGBA_8;
    }
  }

  if (!(colorType & PNG_COLOR_MASK_ALPHA)) {
    png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
  }

  cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);

  useTransform = true;

  transform = cmsCreateTransform(
      src_profile, inType, targetProfile, TYPE_RGBA_8,
      cmsGetHeaderRenderingIntent(src_profile), cmsFLAGS_COPY_ALPHA);

  cmsCloseProfile(src_profile);

  int32_t passes = png_set_interlace_handling(png);

  png_read_update_info(png, pinfo);

  uint32_t inComponents = png_get_channels(png, pinfo);
  uint32_t inStride = info.imageWidth * inComponents;
  uint32_t inStrideOffset = inRect.x * inComponents;

  uint8_t* outPixelsPos = outPixels;
  uint32_t outStride = outRect.width * inComponents;

  auto rowFn = inComponents == 1   ? &GRAY8_to_GRAY8_row
               : inComponents == 2 ? &GRAYA88_to_GRAYA88_row
                                   : &RGBA8888_to_RGBA8888_row;

  if (sampleSize == 1) {
    uint32_t inRemainY = info.imageHeight - inRect.height - inRect.y;

    if (passes == 1) {
      auto inRow = std::vector<uint8_t>(inStride);
      auto* inRowPtr = inRow.data();
      uint8_t* rowToWrite = inRowPtr + inStrideOffset;

      png_skip_rows(png, inRect.y);
      for (uint32_t i = 0; i < inRect.height; ++i) {
        png_read_row(png, inRowPtr, nullptr);
        memcpy(outPixelsPos, rowToWrite, outStride);
        outPixelsPos += outStride;
      }
      png_skip_rows(png, inRemainY);
    } else {
      // decode the image in full
      auto inPixels = std::vector<uint8_t>(inStride * inRect.height);
      auto* inPixelsPos = inPixels.data();

      while (--passes >= 0) {
        png_skip_rows(png, inRect.y);
        for (uint32_t i = 0; i < inRect.height; ++i) {
          png_read_row(png, inPixelsPos, nullptr);
          inPixelsPos += inStride;
        }
        png_skip_rows(png, inRemainY);
        inPixelsPos = inPixels.data();
      }

      for (uint32_t i = 0; i < inRect.height; ++i) {
        memcpy(outPixelsPos, inPixelsPos + inStrideOffset, outStride);
        inPixelsPos += inStride;
        outPixelsPos += outStride;
      }
    }
  } else {
    uint32_t skipStart = (sampleSize - 2) / 2;
    uint32_t skipEnd = sampleSize - 2 - skipStart;

    if (passes == 1) {
      auto inRow1 = std::vector<uint8_t>(inStride);
      auto inRow2 = std::vector<uint8_t>(inStride);

      auto* inRow1Ptr = inRow1.data();
      auto* inRow2Ptr = inRow2.data();
      uint8_t* row1ToWrite = inRow1Ptr + inStrideOffset;
      uint8_t* row2ToWrite = inRow2Ptr + inStrideOffset;

      png_skip_rows(png, inRect.y);

      for (uint32_t i = 0; i < outRect.height; ++i) {
        png_skip_rows(png, skipStart);
        png_read_row(png, inRow1Ptr, nullptr);
        png_read_row(png, inRow2Ptr, nullptr);

        rowFn(outPixelsPos, row1ToWrite, row2ToWrite, outRect.width,
              sampleSize);
        png_skip_rows(png, skipEnd);
        outPixelsPos += outStride;
      }
    } else {
      auto tmpPixels = std::vector<uint8_t>(inStride * outRect.height * 2);

      if (!useTransform) {
        tmpPixels.resize(outRect.width * outRect.height * 8);
      }

      auto* tmpPixelsPos = tmpPixels.data();

      uint32_t inHeightRounded = outRect.height * sampleSize;
      uint32_t inRemainY = info.imageHeight - inHeightRounded - inRect.y;

      while (--passes >= 0) {
        png_skip_rows(png, inRect.y);
        for (uint32_t i = 0; i < outRect.height; ++i) {
          png_skip_rows(png, skipStart);
          png_read_row(png, tmpPixelsPos, nullptr);
          tmpPixelsPos += inStride;
          png_read_row(png, tmpPixelsPos, nullptr);
          tmpPixelsPos += inStride;
          png_skip_rows(png, skipEnd);
        }
        png_skip_rows(png, inRemainY);
        tmpPixelsPos = tmpPixels.data();
      }

      for (uint32_t i = 0; i < outRect.height; ++i) {
        rowFn(outPixelsPos, tmpPixelsPos + inStrideOffset,
              tmpPixelsPos + inStride + inStrideOffset, outRect.width,
              sampleSize);
        outPixelsPos += outStride;
        tmpPixelsPos += inStride * 2;
      }
    }
  }
}
