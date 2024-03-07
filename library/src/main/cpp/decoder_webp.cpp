//
// Created by len on 30/12/20.
//

#include "decoder_webp.h"

WebpDecoder::WebpDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders,
                         cmsHPROFILE targetProfile)
    : BaseDecoder(std::move(stream), cropBorders, targetProfile) {
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

  Rect bounds = {.x = 0, .y = 0, .width = imageWidth, .height = imageHeight};
  if (!isAnimated && cropBorders) {
    int iw = features.width;
    int ih = features.height;
    uint8_t *u, *v;
    int stride, uvStride;
    auto* luma = WebPDecodeYUV(stream->bytes, stream->size, &iw, &ih, &u, &v,
                               &stride, &uvStride);
    if (luma != nullptr) {
      bounds = findBorders(luma, imageWidth, imageHeight);
      WebPFree(luma);
    } else {
      LOGW("Couldn't crop borders on a WebP image of size %dx%d", imageWidth,
           imageHeight);
    }
  }

  return ImageInfo{
      .imageWidth = imageWidth,
      .imageHeight = imageHeight,
      .isAnimated = isAnimated,
      .bounds = bounds,
  };
}

cmsHPROFILE WebpDecoder::getColorProfile() {
  WebPData data = {.bytes = stream->bytes, .size = stream->size};
  WebPDemuxer* const demux = WebPDemux(&data);

  if (!demux) {
    return nullptr;
  }

  uint32_t flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
  WebPChunkIterator chunk_iter;
  cmsHPROFILE src_profile = nullptr;

  if ((flags & ICCP_FLAG) && WebPDemuxGetChunk(demux, "ICCP", 1, &chunk_iter)) {
    src_profile =
        cmsOpenProfileFromMem(chunk_iter.chunk.bytes, chunk_iter.chunk.size);

    WebPDemuxReleaseChunkIterator(&chunk_iter);
  }

  WebPDemuxDelete(demux);

  if (!src_profile) {
    return nullptr;
  }

  cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);

  // WebP doesn't support gray-scale.
  if (profileSpace != cmsSigRgbData) {
    cmsCloseProfile(src_profile);
    return nullptr;
  }

  return src_profile;
}

void WebpDecoder::decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                         uint32_t sampleSize) {
  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);

  cmsHPROFILE src_profile = getColorProfile();
  if (!src_profile) {
    src_profile = cmsCreate_sRGBProfile();
  }

  cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);
  useTransform = true;

  inType = TYPE_RGBA_8;

  transform = cmsCreateTransform(
      src_profile, inType, targetProfile, TYPE_RGBA_8,
      cmsGetHeaderRenderingIntent(src_profile), cmsFLAGS_COPY_ALPHA);

  cmsCloseProfile(src_profile);

  // Set decode region
  config.options.use_cropping =
      inRect.width != info.imageWidth || inRect.height != info.imageHeight;
  config.options.crop_left = inRect.x;
  config.options.crop_top = inRect.y;
  config.options.crop_width = inRect.width;
  config.options.crop_height = inRect.height;

  // Set sample size
  config.options.use_scaling = sampleSize > 1;
  config.options.scaled_width = outRect.width;
  config.options.scaled_height = outRect.height;

  // Set colorspace and stride params
  uint32_t outStride = outRect.width * 4;
  config.output.colorspace = MODE_RGBA;
  config.output.u.RGBA.rgba = outPixels;
  config.output.u.RGBA.size = outStride * outRect.height;
  config.output.u.RGBA.stride = outStride;
  config.output.is_external_memory = 1;

  VP8StatusCode code;
  if (!info.isAnimated) {
    code = WebPDecode(stream->bytes, stream->size, &config);
  } else {
    WebPData data = {.bytes = stream->bytes, .size = stream->size};
    auto demuxer = std::unique_ptr<WebPDemuxer, decltype(&WebPDemuxDelete)>{
        WebPDemux(&data), WebPDemuxDelete};

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
