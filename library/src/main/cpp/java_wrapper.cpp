//
// Created by len on 23/12/20.
//

#include "borders.h"
#include "decoders.h"
#include "java_objects.h"
#include "java_stream.h"
#include <android/bitmap.h>
#include <jni.h>
#include <vector>

#ifdef HAVE_LCMS
#include <include/lcms2.h>
#endif

jint JNI_OnLoad(JavaVM* vm, void*) {
  JNIEnv* env;
  if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
    init_java_stream(env);
    init_java_objects(env);

  } else {
    return JNI_ERR;
  }
  return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeNewInstance(JNIEnv* env, jclass,
                                                      jobject jstream,
                                                      jboolean cropBorders) {
  auto stream = read_all_java_stream(env, jstream);
  if (!stream) {
    return nullptr;
  }

  BaseDecoder* decoder;
  try {
    if (false) {
    } // This should be optimized out by the compiler.
#ifdef HAVE_LIBJPEG
    else if (is_jpeg(stream->bytes)) {
      decoder = new JpegDecoder(std::move(stream), cropBorders);
    }
#endif
#ifdef HAVE_LIBPNG
    else if (is_png(stream->bytes)) {
      decoder = new PngDecoder(std::move(stream), cropBorders);
    }
#endif
#ifdef HAVE_LIBWEBP
    else if (is_webp(stream->bytes)) {
      decoder = new WebpDecoder(std::move(stream), cropBorders);
    }
#endif
#ifdef HAVE_LIBHEIF
    else if (is_libheif_compatible(stream->bytes, stream->size)) {
      decoder = new HeifDecoder(std::move(stream), cropBorders);
    }
#endif
#ifdef HAVE_LIBJXL
    else if (is_jxl(stream->bytes)) {
      decoder = new JpegxlDecoder(std::move(stream), cropBorders);
    }
#endif
    else {
      LOGE("No decoder found to handle this stream");
      return nullptr;
    }
  } catch (std::exception& ex) {
    LOGE("%s", ex.what());
    return nullptr;
  }

  Rect bounds = decoder->info.bounds;
  return create_image_decoder(env, (jlong)decoder, bounds.width, bounds.height);
}

extern "C" JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeDecode(
    JNIEnv* env, jobject, jlong decoderPtr, jboolean rgb565, jint sampleSize,
    jint x, jint y, jint width, jint height, jboolean apply_cms,
    jbyteArray icm_stream) {
  auto* decoder = (BaseDecoder*)decoderPtr;

  // Bounds of the image when crop borders is enabled, otherwise it matches the
  // entire image.
  Rect bounds = decoder->info.bounds;

  // Translated requested bounds to the original image.
  Rect inRect = {x + bounds.x, y + bounds.y, (uint32_t)width, (uint32_t)height};

  // Sampled requested bounds according to sampleSize.
  // It matches the translated bounds when the value is 1
  Rect outRect = inRect.downsample(sampleSize);
  if (outRect.width == 0 || outRect.height == 0) {
    LOGE("Requested sample size too high");
    return nullptr;
  }

  auto* bitmap = create_bitmap(env, outRect.width, outRect.height, rgb565);
  if (!bitmap) {
    LOGE("Failed to create a bitmap of size %dx%dx%d", outRect.width,
         outRect.height, rgb565 ? 2 : 4);
    return nullptr;
  }

  uint8_t* pixels;
  AndroidBitmap_lockPixels(env, bitmap, (void**)&pixels);
  if (!pixels) {
    LOGE("Failed to lock pixels");
    return nullptr;
  }

  try {
    decoder->decode(pixels, outRect, inRect, rgb565, sampleSize);
  } catch (std::exception& ex) {
    LOGE("%s", ex.what());
    AndroidBitmap_unlockPixels(env, bitmap);
    return nullptr;
  }

#ifdef HAVE_LCMS
  auto embedded_icc = decoder->info.icc_profile;

  if (apply_cms && (!embedded_icc.empty() || icm_stream != NULL)) {
    uint8_t* col_buf;
    std::vector<uint8_t> color;
    if (rgb565) {
      color.resize(outRect.width * outRect.height * 4);
      uint16_t* pixels_buf = (uint16_t*)pixels;
      for (int i = 0; i < outRect.width * outRect.height; i++) {
        uint8_t r = (pixels_buf[i] & 0xF800) >> 8;
        uint8_t g = (pixels_buf[i] & 0x07E0) >> 3;
        uint8_t b = (pixels_buf[i] & 0x001F) << 3;
        color[i * 4 + 0] = r;
        color[i * 4 + 1] = g;
        color[i * 4 + 2] = b;
        color[i * 4 + 3] = 255;
      }
      col_buf = color.data();
    } else {
      col_buf = pixels;
    }

    cmsHPROFILE src_profile;
    cmsHPROFILE target_profile;

    if (embedded_icc.empty()) {
      src_profile = cmsCreate_sRGBProfile();
    } else {
      try {
        src_profile =
            cmsOpenProfileFromMem(embedded_icc.data(), embedded_icc.size());
      } catch (std::exception& ex) {
        src_profile = cmsCreate_sRGBProfile();
      }
    }

    if (icm_stream != NULL) {
      int icm_stream_len = env->GetArrayLength(icm_stream);
      try {
        if (icm_stream_len > 0) {
          std::vector<uint8_t> icm_buf(icm_stream_len);
          env->GetByteArrayRegion(icm_stream, 0, icm_stream_len,
                                  reinterpret_cast<jbyte*>(icm_buf.data()));

          target_profile =
              cmsOpenProfileFromMem(icm_buf.data(), icm_buf.size());
        } else {
          target_profile = cmsCreate_sRGBProfile();
        }
      } catch (std::exception& ex) {
        target_profile = cmsCreate_sRGBProfile();
      }
    } else {
      target_profile = cmsCreate_sRGBProfile();
    }

    try {
      auto transform =
          cmsCreateTransform(src_profile, TYPE_RGBA_8, target_profile,
                             TYPE_RGBA_8, INTENT_PERCEPTUAL, 0);

      cmsDoTransform(transform, col_buf, col_buf,
                     outRect.width * outRect.height);

      cmsDeleteTransform(transform);
    } catch (std::exception& ex) {
      LOGE("%s", ex.what());
    }

    cmsCloseProfile(src_profile);
    cmsCloseProfile(target_profile);

    if (rgb565) {
      uint16_t* pixels_buf = (uint16_t*)pixels;
      for (int i = 0; i < outRect.width * outRect.height; i++) {
        uint16_t c = (((col_buf[i * 4 + 0]) << 8) & 0xF800) |
                     (((col_buf[i * 4 + 1]) << 3) & 0x07E0) |
                     (((col_buf[i * 4 + 2]) >> 3) & 0x001F);
        pixels_buf[i] = c;
      }
    }
  }
#endif

  AndroidBitmap_unlockPixels(env, bitmap);
  return bitmap;
}

extern "C" JNIEXPORT void JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeRecycle(JNIEnv*, jobject,
                                                  jlong decoderPtr) {
  auto* decoder = (BaseDecoder*)decoderPtr;
  delete decoder;
}

extern "C" JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeFindType(JNIEnv* env, jclass,
                                                   jbyteArray array) {
  uint32_t toRead = 32;
  uint32_t size = env->GetArrayLength(array);

  if (size < toRead) {
    LOGW("Not enough bytes to parse info");
    return nullptr;
  }

  auto _bytes = std::make_unique<uint8_t[]>(toRead);
  auto bytes = _bytes.get();
  env->GetByteArrayRegion(array, 0, toRead, (jbyte*)bytes);

  if (is_jpeg(bytes)) {
    return create_image_type(env, 0, false);
  } else if (is_png(bytes)) {
    return create_image_type(env, 1, false);
  } else if (is_webp(bytes)) {
    try {
#ifdef HAVE_LIBWEBP
      auto decoder = std::make_unique<WebpDecoder>(
          std::make_shared<Stream>(bytes, size), false);
      return create_image_type(env, 2, decoder->info.isAnimated);
#else
      throw std::runtime_error("WebP decoder not available");
#endif
    } catch (std::exception& ex) {
      LOGW("Failed to parse WebP header. Falling back to non animated WebP");
      return create_image_type(env, 2, false);
    }
  } else if (is_gif(bytes)) {
    return create_image_type(env, 3, true);
  } else if (is_jxl(bytes)) {
    return create_image_type(env, 6, false);
  }

  switch (get_ftyp_image_type(bytes, toRead)) {
  case ftyp_image_type_heif:
    return create_image_type(env, 4, false);
  case ftyp_image_type_avif:
    return create_image_type(env, 5, false);
  case ftyp_image_type_no:
    break;
  }

  LOGW("Failed to find image type");
  return nullptr;
}
