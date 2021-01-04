//
// Created by len on 23/12/20.
//

#include "decoder_jpeg.h"

bool JpegDecoder::handles(const uint8_t* stream) {
  return stream[0] == 0xFF && stream[1] == 0xD8 && stream[2] == 0xFF;
}

JpegDecoder::JpegDecoder(
  std::unique_ptr<Stream>&& stream,
  bool cropBorders
) : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

JpegDecodeSession::JpegDecodeSession() : jinfo(jpeg_decompress_struct{}), jerr(jpeg_error_mgr{}) {
}

void JpegDecodeSession::init(Stream *stream) {
  jinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = [](j_common_ptr info){
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    (*(info->err->format_message))(info, jpegLastErrorMsg);
    throw std::runtime_error(jpegLastErrorMsg);
  };

  jpeg_create_decompress(&jinfo);
  jpeg_mem_src(&jinfo, stream->bytes, stream->size);
  jpeg_read_header(&jinfo, true);
}

JpegDecodeSession::~JpegDecodeSession() {
  jpeg_destroy_decompress(&jinfo);
}

std::unique_ptr<JpegDecodeSession> JpegDecoder::initDecodeSession() {
  auto session = std::make_unique<JpegDecodeSession>();
  session->init(stream.get());
  return session;
}

ImageInfo JpegDecoder::parseInfo() {
  auto session = initDecodeSession();
  auto jinfo = session->jinfo;

  Rect bounds{};
  if (cropBorders) {
    jinfo.out_color_space = JCS_GRAYSCALE;
    jpeg_start_decompress(&jinfo);

    auto pixels = std::make_unique<uint8_t[]>(jinfo.image_width * jinfo.image_height);
    uint8_t* pixelsPtr = pixels.get();
    while (jinfo.output_scanline < jinfo.output_height) {
      uint8_t* offset = pixelsPtr + jinfo.output_scanline * jinfo.image_width;
      jpeg_read_scanlines(&jinfo, &offset, 1);
    }
    bounds = findBorders(pixels.get(), jinfo.image_width, jinfo.image_height);
    jpeg_finish_decompress(&jinfo);
  } else {
    bounds = Rect { .x = 0, .y = 0, .width = jinfo.image_width, .height = jinfo.image_height };
  }

  ImageInfo info {
    .imageWidth = jinfo.image_width,
    .imageHeight = jinfo.image_height,
    .isAnimated = false,
    .bounds = bounds
  };

  return info;
}

void JpegDecoder::decode(uint8_t* outPixels, Rect outRect, Rect, bool rgb565, uint32_t sampleSize) {
  auto session = initDecodeSession();
  auto jinfo = session->jinfo;

  jinfo.scale_denom = sampleSize;
  jinfo.out_color_space = rgb565 ? JCS_RGB565 : JCS_EXT_RGBA;
  if (rgb565) {
    jinfo.dither_mode = JDITHER_NONE;
  }

  jpeg_start_decompress(&jinfo);

  uint32_t pixelSize = rgb565 ? 2 : 4;
  uint32_t inX = outRect.x;
  uint32_t inWidth = outRect.width;
  uint32_t outStride = outRect.width * pixelSize;

  jpeg_crop_scanline(&jinfo, &inX, &inWidth);
  jpeg_skip_scanlines(&jinfo, outRect.y);

  // This has to be called after jpeg_crop_scanline as inWidth might change
  uint32_t inStride = inWidth * pixelSize;

  auto inPixels = std::make_unique<uint8_t[]>(inStride);

  uint8_t* inPixelsPos = inPixels.get();
  // libjpeg doesn't always provide the exact requested region because it has to be a multiple of
  // the DCT, so we have to account for shifts.
  uint8_t* inPixelsPosAligned = inPixelsPos + (outRect.x - inX) * pixelSize;
  uint8_t* outPixelsPos = outPixels;

  for (uint32_t i = 0; i < outRect.height; i++) {
    jpeg_read_scanlines(&jinfo, &inPixelsPos, 1);
    memcpy(outPixelsPos, inPixelsPosAligned, outStride);
    outPixelsPos += outStride;
  }

  jpeg_skip_scanlines(&jinfo, outRect.height - outRect.y);
  jpeg_finish_decompress(&jinfo);
}
