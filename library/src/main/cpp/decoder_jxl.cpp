//
// Created by w on 02/07/21.
//

#include "decoder_jxl.h"
#include "row_convert.h"

JpegxlDecoder::JpegxlDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders)
    : BaseDecoder(std::move(stream), cropBorders), mSrcProfile(nullptr) {
  this->info = parseInfo();
}

void JpegxlDecoder::decode() {
  if (!pixels.empty()) {
    return;
  }

  auto runner = JxlResizableParallelRunnerMake(nullptr);

  auto dec = JxlDecoderMake(nullptr);
  if (JXL_DEC_SUCCESS !=
      JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO |
                                               JXL_DEC_COLOR_ENCODING |
                                               JXL_DEC_FULL_IMAGE)) {
    throw std::runtime_error("JxlDecoderSubscribeEvents failed");
  }

  if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
                                                     JxlResizableParallelRunner,
                                                     runner.get())) {
    throw std::runtime_error("JxlDecoderSetParallelRunner failed");
  }

  JxlDecoderSetInput(dec.get(), stream->bytes, stream->size);
  JxlDecoderCloseInput(dec.get());

  JxlPixelFormat format = {4, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0};
  for (;;) {
    JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

    if (status == JXL_DEC_ERROR) {
      throw std::runtime_error("Decoder error");
    } else if (status == JXL_DEC_NEED_MORE_INPUT) {
      throw std::runtime_error("Error, already provided all input");
    } else if (status == JXL_DEC_BASIC_INFO) {
      if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &jxl_info)) {
        throw std::runtime_error("JxlDecoderGetBasicInfo failed");
      }
      JxlResizableParallelRunnerSetThreads(
          runner.get(), JxlResizableParallelRunnerSuggestThreads(
                            jxl_info.xsize, jxl_info.ysize));
    } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
      size_t buffer_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
        throw std::runtime_error("JxlDecoderImageOutBufferSize failed");
      }

      pixels.resize(buffer_size);
      if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                         pixels.data(),
                                                         buffer_size)) {
        throw std::runtime_error("JxlDecoderSetImageOutBuffer failed");
      }
    } else if (status == JXL_DEC_COLOR_ENCODING) {
      size_t size = 0;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderGetICCProfileSize(dec.get(), JXL_COLOR_PROFILE_TARGET_DATA,
                                      &size)) {
        throw std::runtime_error("JxlDecoderGetICCProfileSize failed");
      }

      std::vector<uint8_t> icc_profile(size);
      if (JXL_DEC_SUCCESS != JxlDecoderGetColorAsICCProfile(
                                 dec.get(), JXL_COLOR_PROFILE_TARGET_DATA,
                                 icc_profile.data(), size)) {
        throw std::runtime_error("JxlDecoderGetColorAsICCProfile failed");
      }

      mSrcProfile = cmsOpenProfileFromMem(icc_profile.data(), size);
      cmsColorSpaceSignature profileSpace = cmsGetColorSpace(mSrcProfile);

      if (profileSpace != cmsSigRgbData && (jxl_info.num_color_channels == 3 ||
                                            profileSpace != cmsSigGrayData)) {
        cmsCloseProfile(mSrcProfile);
        mSrcProfile = nullptr;
        break;
      }
    } else if (status == JXL_DEC_FULL_IMAGE) {
      break;
    } else if (status == JXL_DEC_SUCCESS) {
      break;
    } else {
      LOGW("Unexpected decoder status");
      break;
    }
  }
}

ImageInfo JpegxlDecoder::parseInfo() {
  decode();

  Rect bounds;
  if (cropBorders) {
    std::vector<uint8_t> gray_pixels(jxl_info.xsize * jxl_info.ysize);
    uint8_t* gray_buffer = gray_pixels.data();
    if (jxl_info.num_color_channels == 3) {
      for (int x = 0; x < pixels.size(); x += 4) {
        uint8_t r = pixels[x + 0];
        uint8_t g = pixels[x + 1];
        uint8_t b = pixels[x + 2];
        int gray = (r * 0.2126 + g * 0.7152 + b * 0.0722) + 0.5;
        if (gray > 255) {
          gray = 255;
        }
        gray_buffer[x / 4] = (uint8_t)gray;
      }
    } else {
      for (int x = 0; x < pixels.size(); x += 4) {
        gray_buffer[x / 4] = pixels[x];
      }
    }

    bounds = findBorders(gray_buffer, jxl_info.xsize, jxl_info.ysize);
  } else {
    bounds = {
        .x = 0, .y = 0, .width = jxl_info.xsize, .height = jxl_info.ysize};
  }

  return ImageInfo{
      .imageWidth = jxl_info.xsize,
      .imageHeight = jxl_info.ysize,
      .isAnimated = false, // (bool)jxl_info.have_animation,
      .bounds = bounds,
  };
}

void JpegxlDecoder::decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                           uint32_t sampleSize, cmsHPROFILE targetProfile) {
  decode();

  // Save transformed pixel data.
  if (!transformed) {
    uint8_t* buf = pixels.data();

    cmsUInt32Number inType;
    std::vector<uint8_t> gray;
    if (jxl_info.num_color_channels == 3) {
      inType = TYPE_RGBA_8;
    } else {
      uint32_t num_pixels = jxl_info.xsize * jxl_info.ysize;
      if (jxl_info.alpha_bits > 0) {
        gray.resize(num_pixels * 2);
        for (int i = 0; i < num_pixels; i++) {
          gray[i * 2] = buf[i * 4];
          gray[i * 2 + 1] = buf[i * 4 + 3];
        }
        inType = TYPE_GRAYA_8;
      } else {
        gray.resize(num_pixels);
        for (int i = 0; i < num_pixels; i++) {
          gray[i] = buf[i * 4];
        }
        inType = TYPE_GRAY_8;
      }
      buf = gray.data();
    }

    if (!mSrcProfile) {
      mSrcProfile = cmsCreate_sRGBProfile(); // assume sRGB, should never happen
    }

    transform =
        cmsCreateTransform(mSrcProfile, inType, targetProfile, TYPE_RGBA_8,
                           cmsGetHeaderRenderingIntent(mSrcProfile), 0);

    cmsCloseProfile(mSrcProfile);

    cmsDoTransform(transform, buf, pixels.data(),
                   info.imageWidth * info.imageHeight);

    cmsDeleteTransform(transform);
    transform = nullptr;
    transformed = true;
  }

  // Copied from decoder_heif.cpp
  uint32_t inStride = info.imageWidth * 4;
  uint32_t inStrideOffset = inRect.x * (inStride / info.imageWidth);
  auto inPixelsPos = (uint8_t*)pixels.data() + inStride * inRect.y;

  // Calculate output stride
  uint32_t outStride = outRect.width * 4;
  uint8_t* outPixelsPos = outPixels;

  // Set row conversion function
  auto rowFn = &RGBA8888_to_RGBA8888_row;

  if (sampleSize == 1) {
    for (uint32_t i = 0; i < outRect.height; i++) {
      // Apply row conversion function to the following row
      rowFn(outPixelsPos, inPixelsPos + inStrideOffset, nullptr, outRect.width,
            1);

      // Shift row to read and write
      inPixelsPos += inStride;
      outPixelsPos += outStride;
    }
  } else {
    // Calculate the number of rows to discard
    uint32_t skipStart = (sampleSize - 2) / 2;
    uint32_t skipEnd = sampleSize - 2 - skipStart;

    for (uint32_t i = 0; i < outRect.height; ++i) {
      // Skip starting rows
      inPixelsPos += inStride * skipStart;

      // Apply row conversion function to the following two rows
      rowFn(outPixelsPos, inPixelsPos + inStrideOffset,
            inPixelsPos + inStrideOffset + inStride, outRect.width, sampleSize);

      // Shift row to read to the next 2 rows (the ones we've just read) + the
      // skipped end rows
      inPixelsPos += inStride * (2 + skipEnd);

      // Shift row to write
      outPixelsPos += outStride;
    }
  }
}
