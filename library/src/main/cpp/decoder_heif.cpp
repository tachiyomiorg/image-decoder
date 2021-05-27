//
// Created by len on 27/5/21.
//

#include "decoder_heif.h"

bool HeifDecoder::handles(const uint8_t* stream) {
  // TODO hardcoding length is a bad idea. Also we'll need to know if the file is AVIF or HEIF
  auto result = heif_check_filetype(stream, 32);
  return result == heif_filetype_yes_supported || result == heif_filetype_maybe;
}

HeifDecoder::HeifDecoder(
  std::unique_ptr<Stream>&& stream,
  bool cropBorders
) : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

ImageInfo HeifDecoder::parseInfo() {
  heif_context* ctx = heif_context_alloc();
  heif_context_read_from_memory_without_copy(ctx, stream->bytes, stream->size, nullptr);

  heif_image_handle* handle;
  heif_context_get_primary_image_handle(ctx, &handle);

  uint32_t imageWidth = heif_image_handle_get_width(handle);
  uint32_t imageHeight = heif_image_handle_get_height(handle);
  bool isAnimated = false;
  Rect bounds = { .x = 0, .y = 0, .width = imageWidth, .height = imageHeight };

  heif_image_handle_release(handle);
  heif_context_free(ctx);

  return ImageInfo {
    .imageWidth = imageWidth,
    .imageHeight = imageHeight,
    .isAnimated = isAnimated,
    .bounds = bounds
  };
}

void HeifDecoder::decode(uint8_t *outPixels, Rect outRect, Rect inRect, bool rgb565, uint32_t sampleSize) {
  heif_context* ctx = heif_context_alloc();
  heif_context_read_from_memory_without_copy(ctx, stream->bytes, stream->size, nullptr);

  heif_image_handle* handle;
  heif_context_get_primary_image_handle(ctx, &handle);

  heif_image* img;
  heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);

  int stride;
  const uint8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

  uint32_t height = heif_image_get_primary_height(img);
  memcpy(outPixels, data, stride * height);

  heif_image_handle_release(handle);
  heif_context_free(ctx);
}
