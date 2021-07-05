//
// Created by w on 02/07/21.
//

#include "decoder_jxl.h"
#include "row_convert.h"
#include <vector>

JpegxlDecoder::JpegxlDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders)
    : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

std::vector<uint8_t> decodeRGB(const uint8_t* data, size_t size,
                               JxlBasicInfo* info, uint32_t channels = 0) {
  auto runner = JxlResizableParallelRunnerMake(nullptr);

  auto dec = JxlDecoderMake(nullptr);
  if (JXL_DEC_SUCCESS !=
      JxlDecoderSubscribeEvents(dec.get(),
                                JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE)) {
    throw std::runtime_error("JxlDecoderSubscribeEvents failed");
  }

  if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
                                                     JxlResizableParallelRunner,
                                                     runner.get())) {
    throw std::runtime_error("JxlDecoderSetParallelRunner failed");
  }

  JxlDecoderSetInput(dec.get(), data, size);

  std::vector<uint8_t> pixels;
  for (;;) {
    JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

    if (status == JXL_DEC_ERROR) {
      throw std::runtime_error("Decoder error");
    } else if (status == JXL_DEC_NEED_MORE_INPUT) {
      throw std::runtime_error("Error, already provided all input");
    } else if (status == JXL_DEC_BASIC_INFO) {
      if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), info)) {
        throw std::runtime_error("JxlDecoderGetBasicInfo failed");
      }
      JxlResizableParallelRunnerSetThreads(
          runner.get(),
          JxlResizableParallelRunnerSuggestThreads(info->xsize, info->ysize));
    } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
      if (channels == 0) {
        channels = (info->alpha_bits > 0 ? 1 : 0) + info->num_color_channels;
      }
      JxlPixelFormat format = {channels, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

      size_t buffer_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
        throw std::runtime_error("JxlDecoderImageOutBufferSize failed");
      }

      pixels.resize(buffer_size);
      if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                         pixels.data(),
                                                         pixels.size())) {
        throw std::runtime_error("JxlDecoderSetImageOutBuffer failed");
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

  return pixels;
}

ImageInfo JpegxlDecoder::parseInfo() {
  Rect bounds;
  JxlBasicInfo jxl_info;

  if (cropBorders) {
    std::vector<uint8_t> pixels =
        decodeRGB(stream->bytes, stream->size, &jxl_info);

    uint8_t* pixels_buffer = (uint8_t*)pixels.data();
    uint32_t num_channels =
        (jxl_info.alpha_bits > 0 ? 1 : 0) + jxl_info.num_color_channels;

    // Re-align gray image to the front of the vector
    if (num_channels == 2) {
      for (int x = 0; x < pixels.size(); x += num_channels) {
        pixels_buffer[x / num_channels] = pixels_buffer[x];
      }
    } else if (num_channels == 3 || num_channels == 4) {
      for (int x = 0; x < pixels.size(); x += num_channels) {
        uint8_t r = pixels_buffer[x + 0];
        uint8_t g = pixels_buffer[x + 1];
        uint8_t b = pixels_buffer[x + 2];
        int gray = (r * 0.2126 + g * 0.7152 + b * 0.0722) + 0.5;
        if (gray > 255) {
          gray = 255;
        }
        pixels_buffer[x / num_channels] = (uint8_t)gray;
      }
    }

    bounds = findBorders(pixels.data(), jxl_info.xsize, jxl_info.ysize);
  } else {
    auto dec = JxlDecoderMake(NULL);

    if (JXL_DEC_SUCCESS !=
        JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO)) {
      throw std::runtime_error("JxlDecoderSubscribeEvents failed");
    }

    JxlDecoderSetInput(dec.get(), stream->bytes, stream->size);

    for (;;) {
      JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

      if (status == JXL_DEC_ERROR) {
        throw std::runtime_error("Decoder error");
      } else if (status == JXL_DEC_NEED_MORE_INPUT) {
        throw std::runtime_error("Already provided all input");
      } else if (status == JXL_DEC_BASIC_INFO) {
        if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &jxl_info)) {
          throw std::runtime_error("JxlDecoderGetBasicInfo failed");
        }
        break;
      } else if (status == JXL_DEC_SUCCESS) {
        break;
      } else {
        LOGW("Unexpected decoder status");
        break;
      }
    }

    bounds = {
        .x = 0, .y = 0, .width = jxl_info.xsize, .height = jxl_info.ysize};
  }

  return ImageInfo{.imageWidth = jxl_info.xsize,
                   .imageHeight = jxl_info.ysize,
                   .isAnimated = false, // (bool)jxl_info.have_animation,
                   .bounds = bounds};
}

void JpegxlDecoder::decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                           bool rgb565, uint32_t sampleSize) {
  JxlBasicInfo jxl_info;
  std::vector<uint8_t> pixels =
      decodeRGB(stream->bytes, stream->size, &jxl_info, 4);

  // Copied from decoder_heif.cpp
  int stride = 4 * info.imageWidth;
  uint32_t inStride = stride;
  uint32_t inStrideOffset = inRect.x * (stride / info.imageWidth);
  auto inPixelsPos = (uint8_t*)pixels.data() + inStride * inRect.y;

  // Calculate output stride
  uint32_t outStride = outRect.width * (rgb565 ? 2 : 4);
  uint8_t* outPixelsPos = outPixels;

  // Set row conversion function
  auto rowFn = rgb565 ? &RGBA8888_to_RGB565_row : &RGBA8888_to_RGBA8888_row;

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
            inPixelsPos + inStride + inStrideOffset, outRect.width, sampleSize);

      // Shift row to read to the next 2 rows (the ones we've just read) + the
      // skipped end rows
      inPixelsPos += inStride * (2 + skipEnd);

      // Shift row to write
      outPixelsPos += outStride;
    }
  }
}
