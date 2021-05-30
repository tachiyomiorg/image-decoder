//
// Created by len on 30/12/20.
//

#include "decoder_webp.h"

WebpDecoder::WebpDecoder(
  std::shared_ptr<Stream>&& stream,
  bool cropBorders
) : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

ImageInfo WebpDecoder::parseInfo() {
  WebPBitstreamFeatures features;
  if (WebPGetFeatures(stream->bytes, 32, &features) != VP8_STATUS_OK) {
    throw std::runtime_error("Failed to parse webp");
  }

  uint32_t imageWidth = features.width;
  uint32_t imageHeight = features.height;
  bool isAnimated = features.has_animation;

  Rect bounds = { .x = 0, .y = 0, .width = imageWidth, .height = imageHeight };
  if (!isAnimated && cropBorders) {
    int iw = features.width;
    int ih = features.height;
    uint8_t *u, *v;
    int stride, uvStride;
    auto* luma = WebPDecodeYUV(stream->bytes, stream->size, &iw, &ih, &u, &v, &stride, &uvStride);
    if (luma != nullptr) {
      bounds = findBorders(luma, imageWidth, imageHeight);
      WebPFree(luma);
    } else {
      LOGW("Couldn't crop borders on a WebP image of size %dx%d", imageWidth, imageHeight);
    }
  }

  return ImageInfo {
    .imageWidth = imageWidth,
    .imageHeight = imageHeight,
    .isAnimated = isAnimated,
    .bounds = bounds
  };
}

void WebpDecoder::decode(uint8_t *outPixels, Rect outRect, Rect inRect, bool rgb565, uint32_t sampleSize) {
  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);

  // Set decode region
  config.options.use_cropping = inRect.width != info.imageWidth || inRect.height != info.imageHeight;
  config.options.crop_left = inRect.x;
  config.options.crop_top = inRect.y;
  config.options.crop_width = inRect.width;
  config.options.crop_height = inRect.height;

  // Set sample size
  config.options.use_scaling = sampleSize > 1;
  config.options.scaled_width = outRect.width;
  config.options.scaled_height = outRect.height;

  // Set colorspace and stride params
  uint32_t outStride = outRect.width * (rgb565 ? 2 : 4);
  config.output.colorspace = rgb565 ? MODE_RGB_565 : MODE_RGBA;
  config.output.u.RGBA.rgba = outPixels;
  config.output.u.RGBA.size = outStride * outRect.height;
  config.output.u.RGBA.stride = outStride;
  config.output.is_external_memory = 1;

  VP8StatusCode code;
  if (!info.isAnimated) {
    code = WebPDecode(stream->bytes, stream->size, &config);
  } else {
    WebPData data = {.bytes = stream->bytes, .size = stream->size};
    auto demuxer = std::unique_ptr<WebPDemuxer, decltype(&WebPDemuxDelete)> {
      WebPDemux(&data),
      WebPDemuxDelete
    };

    WebPIterator iterator;
    if (!WebPDemuxGetFrame(demuxer.get(), 1, &iterator)) {
      throw std::runtime_error("Failed to init iterator");
    }
    code = WebPDecode(iterator.fragment.bytes, iterator.fragment.size, &config);
    WebPDemuxReleaseIterator(&iterator);
  }
  if (code != VP8_STATUS_OK) {
    throw std::runtime_error("Failed to decode image");
  }
}
