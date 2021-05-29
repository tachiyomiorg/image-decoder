//
// Created by len on 27/5/21.
//

#include "decoder_heif.h"
#include <libheif/heif_cxx.h>
#include "row_convert.h"

static const uint8_t magic[] =
  {'f', 't', 'y', 'p'};

static const uint8_t brands[][4] = {
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

bool HeifDecoder::handles(const uint8_t* stream) {
  if (memcmp(stream + 4, magic, 4) != 0) {
    return false;
  }

  for (uint32_t i = 0; i < sizeof(brands); i++) {
    if (memcmp(stream + 8, brands[i], 4) == 0) {
      return true;
    }
  }
  return false;
}

auto init_heif_context(Stream* stream) {
  auto ctx = heif::Context();
  ctx.read_from_memory_without_copy(stream->bytes, stream->size);
  return ctx;
}

HeifDecoder::HeifDecoder(
  std::shared_ptr<Stream>&& stream,
  bool cropBorders
) : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

ImageInfo HeifDecoder::parseInfo() {
  auto ctx = init_heif_context(stream.get());
  auto handle = ctx.get_primary_image_handle();

  uint32_t imageWidth = handle.get_width();
  uint32_t imageHeight = handle.get_height();
  Rect bounds = { .x = 0, .y = 0, .width = imageWidth, .height = imageHeight };
  if (cropBorders) {
    try {
      auto img = handle.decode_image(heif_colorspace_YCbCr, heif_chroma_undefined);
      auto pixels = img.get_plane(heif_channel_Y, nullptr);

      bounds = findBorders(pixels, imageWidth, imageHeight);
    } catch (std::exception &ex) {
      LOGW("Couldn't crop borders on a HEIF/AVIF image of size %dx%d", imageWidth, imageHeight);
    }
  }

  return ImageInfo {
    .imageWidth = imageWidth,
    .imageHeight = imageHeight,
    .isAnimated = false,
    .bounds = bounds
  };
}

void HeifDecoder::decode(uint8_t *outPixels, Rect outRect, Rect inRect, bool rgb565, uint32_t sampleSize) {
  // Decode full image (regions, subsamples or row by row are not supported sadly)
  auto ctx = init_heif_context(stream.get());
  auto handle = ctx.get_primary_image_handle();
  auto img = handle.decode_image(heif_colorspace_RGB, heif_chroma_interleaved_RGBA);

  int stride;
  const uint8_t* inPixels = img.get_plane(heif_channel_interleaved, &stride);

  // Calculate stride of the decoded image with the requested region
  uint32_t inStride = stride;
  uint32_t inStrideOffset = inRect.x * 4;
  auto inPixelsPos = (uint8_t*) inPixels + inStride * inRect.y;

  // Calculate output stride
  uint32_t outStride = outRect.width * (rgb565 ? 2 : 4);
  uint8_t* outPixelsPos = outPixels;

  // Set row conversion function
  // TODO if we add a RGB888 to RG565 we'd save 1/4 of memory
  auto rowFn = rgb565 ? &RGBA8888_to_RGB565_row : &RGBA8888_to_RGBA8888_row;

  if (sampleSize == 1) {
    for (uint32_t i = 0; i < outRect.height; i++) {
      // Apply row conversion function to the following row
      rowFn(outPixelsPos, inPixelsPos + inStrideOffset, nullptr, outRect.width, 1);

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
      rowFn(outPixelsPos, inPixelsPos + inStrideOffset, inPixelsPos + inStride + inStrideOffset, outRect.width, sampleSize);

      // Shift row to read to the next 2 rows (the ones we've just read) + the skipped end rows
      inPixelsPos += inStride * (2 + skipEnd);

      // Shift row to write
      outPixelsPos += outStride;
    }
  }
}
