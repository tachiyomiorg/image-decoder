//
// Created by len on 23/12/20.
//

#include "decoder_jpeg.h"
#include "row_convert.h"
#include <algorithm>

JpegDecoder::JpegDecoder(std::shared_ptr<Stream>&& stream, bool cropBorders)
    : BaseDecoder(std::move(stream), cropBorders) {
  this->info = parseInfo();
}

JpegDecodeSession::JpegDecodeSession()
    : jinfo(jpeg_decompress_struct{}), jerr(jpeg_error_mgr{}) {
  jinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = [](j_common_ptr info) {
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    (*(info->err->format_message))(info, jpegLastErrorMsg);
    throw std::runtime_error(jpegLastErrorMsg);
  };
}

void JpegDecodeSession::init(Stream* stream) {
  jpeg_create_decompress(&jinfo);
  jpeg_mem_src(&jinfo, stream->bytes, stream->size);
  jpeg_save_markers(&jinfo, JPEG_APP0 + 2, 0xFFFF);
  jpeg_read_header(&jinfo, true);
}

JpegDecodeSession::~JpegDecodeSession() { jpeg_destroy_decompress(&jinfo); }

std::unique_ptr<JpegDecodeSession> JpegDecoder::initDecodeSession() {
  auto session = std::make_unique<JpegDecodeSession>();
  session->init(stream.get());
  return session;
}

ImageInfo JpegDecoder::parseInfo() {
  auto session = initDecodeSession();
  auto jinfo = session->jinfo;

  uint32_t imageWidth = jinfo.image_width;
  uint32_t imageHeight = jinfo.image_height;

  Rect bounds = {.x = 0, .y = 0, .width = imageWidth, .height = imageHeight};
  if (cropBorders) {
    try {
      auto pixels = std::make_unique<uint8_t[]>(imageWidth * imageHeight);

      jinfo.out_color_space = JCS_GRAYSCALE;
      jpeg_start_decompress(&jinfo);

      uint8_t* pixelsPtr = pixels.get();
      while (jinfo.output_scanline < jinfo.output_height) {
        uint8_t* offset = pixelsPtr + jinfo.output_scanline * imageWidth;
        jpeg_read_scanlines(&jinfo, &offset, 1);
      }
      jpeg_finish_decompress(&jinfo);
      bounds = findBorders(pixels.get(), imageWidth, imageHeight);
    } catch (std::exception& ex) {
      LOGW("Couldn't crop borders on a JPEG image of size %dx%d", imageWidth,
           imageHeight);
    }
  }

  return ImageInfo{
      .imageWidth = jinfo.image_width,
      .imageHeight = jinfo.image_height,
      .isAnimated = false,
      .bounds = bounds,
  };
}

cmsHPROFILE JpegDecoder::getColorProfile(jpeg_decompress_struct* jinfo) {
  JOCTET* icc_data;
  unsigned int icc_size;
  if (jpeg_read_icc_profile(jinfo, &icc_data, &icc_size)) {
    cmsHPROFILE src_profile = cmsOpenProfileFromMem(icc_data, icc_size);
    free(icc_data);

    cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);

    if (profileSpace != cmsSigRgbData &&
        (jinfo->jpeg_color_space != JCS_GRAYSCALE ||
         profileSpace != cmsSigGrayData)) {
      cmsCloseProfile(src_profile);
      return nullptr;
    }

    return src_profile;
  } else {
    return cmsCreate_sRGBProfile();
  }
}

void JpegDecoder::decode(uint8_t* outPixels, Rect outRect, Rect inRect,
                         bool rgb565, uint32_t sampleSize,
                         cmsHPROFILE targetProfile) {
  auto session = initDecodeSession();
  auto* jinfo = &session->jinfo;

  if (targetProfile) {
    cmsHPROFILE src_profile = getColorProfile(jinfo);
    if (src_profile) {
      cmsColorSpaceSignature profileSpace = cmsGetColorSpace(src_profile);
      useTransform = profileSpace == cmsSigRgbData;

      cmsUInt32Number inType;
      if (useTransform) {
        inType = TYPE_RGBA_8;
      } else {
        inType = TYPE_GRAY_8;
      }

      transform =
          cmsCreateTransform(src_profile, inType, targetProfile, TYPE_RGBA_8,
                             cmsGetHeaderRenderingIntent(src_profile), 0);

      cmsCloseProfile(src_profile);

      jinfo->out_color_space =
          inType == TYPE_GRAY_8 ? JCS_GRAYSCALE : JCS_EXT_RGBA;
    }
  }

  if (!transform) {
    // Set output color space.
    jinfo->out_color_space = rgb565 ? JCS_RGB565 : JCS_EXT_RGBA;
    if (rgb565) {
      jinfo->dither_mode = JDITHER_NONE;
    }
  }

  // 8 is the maximum supported scale by libjpeg, so we'll use custom logic for
  // further samples.
  jinfo->scale_denom = std::min(sampleSize, 8u);
  uint32_t customSample = sampleSize <= 8 ? 0 : sampleSize / 8;
  Rect decRect = customSample == 0 ? outRect : outRect.upsample(customSample);

  // libjpeg X axis need to be a multiple of the defined DCT. We need to know
  // these values in order to crop the unwanted region of the final image.
  uint32_t decX = decRect.x;
  uint32_t decWidth = decRect.width;

  // Init decoder and set regions.
  jpeg_start_decompress(jinfo);
  jpeg_crop_scanline(jinfo, &decX, &decWidth);
  jpeg_skip_scanlines(jinfo, decRect.y);

  // Now that we know the decoding width, we can calculate the row stride.
  uint32_t inPixelSize = jinfo->out_color_space == JCS_GRAYSCALE ? 1
                         : jinfo->out_color_space == JCS_RGB565  ? 2
                                                                 : 4;
  uint32_t inStride = decWidth * inPixelSize;
  uint32_t inStartStride = (decRect.x - decX) * inPixelSize;

  // Allocate row and get pointers to the row and the row aligned for output
  // image.
  auto inRow = std::make_unique<uint8_t[]>(inStride);
  uint8_t* inRowPtr = inRow.get();
  uint8_t* inRowPtrAligned = inRowPtr + inStartStride;

  // Save a pointer to the output image and calculate the output stride.
  uint32_t outPixelSize = (!transform && rgb565) ? 2 : 4;
  uint8_t* outPixelsPos = outPixels;
  uint32_t outStride = outRect.width * outPixelSize;

  if (customSample == 0) {
    std::vector<uint8_t> CMSLine;
    if (!useTransform && transform) {
      CMSLine.resize(outStride);
    }

    // No custom sampler needed, just decode and copy to destination.
    for (uint32_t i = 0; i < outRect.height; i++) {
      jpeg_read_scanlines(jinfo, &inRowPtr, 1);

      auto rowToWrite = inRowPtrAligned;

      if (!useTransform && transform) {
        cmsDoTransform(transform, rowToWrite, CMSLine.data(), outRect.width);
        rowToWrite = CMSLine.data();
      }

      memcpy(outPixelsPos, rowToWrite, outStride);
      outPixelsPos += outStride;
    }
  } else {
    std::vector<uint8_t> CMSLine1;
    std::vector<uint8_t> CMSLine2;
    if (!useTransform && transform) {
      CMSLine1.resize(4 * outRect.width);
      CMSLine2.resize(4 * outRect.width);
    }

    // Custom sampler needed, we need to decode two rows and downsample them.
    // Allocate second row and get pointers to the row and the row aligned for
    // output image.
    auto inRow2 = std::make_unique<uint8_t[]>(inStride);
    uint8_t* inRow2Ptr = inRow2.get();
    uint8_t* inRow2PtrAligned = inRow2Ptr + inStartStride;

    // We'll only decode the middle rows, so skip the other ones
    uint32_t skipStart = (customSample - 2) / 2;
    uint32_t skipEnd = customSample - 2 - skipStart;

    auto rowFn = (transform || !rgb565) ? &RGBA8888_to_RGBA8888_row
                                        : &RGB565_to_RGB565_row;

    for (uint32_t i = 0; i < outRect.height; i++) {
      jpeg_skip_scanlines(jinfo, skipStart);

      jpeg_read_scanlines(jinfo, &inRowPtr, 1);
      jpeg_read_scanlines(jinfo, &inRow2Ptr, 1);

      auto row1ToWrite = inRowPtrAligned;
      auto row2ToWrite = inRow2PtrAligned;

      if (!useTransform && transform) {
        cmsDoTransform(transform, row1ToWrite, CMSLine1.data(), outRect.width);
        cmsDoTransform(transform, row2ToWrite, CMSLine2.data(), outRect.width);
        row1ToWrite = CMSLine1.data();
        row2ToWrite = CMSLine2.data();
      }

      rowFn(outPixelsPos, row1ToWrite, row2ToWrite, outRect.width,
            customSample);
      outPixelsPos += outStride;

      jpeg_skip_scanlines(jinfo, skipEnd);
    }
  }

  // Call abort instead of finish to allow finishing before the last scanline.
  jpeg_abort_decompress(jinfo);
}
