//
// Created by len on 23/12/20.
//

#include <jni.h>
#include <android/bitmap.h>
#include "java_stream.h"
#include "java_objects.h"
#include "decoder_base.h"
#include "decoder_jpeg.h"
#include "decoder_png.h"
#include "decoder_webp.h"
#include "decoder_gif.h"
#include "decoder_heif.h"
#include "borders.h"

jint JNI_OnLoad(JavaVM* vm, void*) {
  JNIEnv* env;
  if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
    init_java_stream(env);
    init_java_objects(env);

  } else {
    return JNI_ERR;
  }
  return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeNewInstance(
  JNIEnv* env, jclass, jobject jstream, jboolean cropBorders
) {
  auto stream = read_all_java_stream(env, jstream);
  if (!stream) {
    return nullptr;
  }

  BaseDecoder* decoder;
  try {
    if (JpegDecoder::handles(stream->bytes)) {
      decoder = new JpegDecoder(std::move(stream), cropBorders);
    } else if (PngDecoder::handles(stream->bytes)) {
      decoder = new PngDecoder(std::move(stream), cropBorders);
    } else if (WebpDecoder::handles(stream->bytes)) {
      decoder = new WebpDecoder(std::move(stream), cropBorders);
    } else if (HeifDecoder::handles(stream->bytes)) {
      decoder = new HeifDecoder(std::move(stream), cropBorders);
    } else {
      LOGE("No decoder found to handle this stream");
      return nullptr;
    }
  } catch (std::exception &ex) {
    LOGE("%s", ex.what());
    return nullptr;
  }

  Rect bounds = decoder->info.bounds;
  return create_image_decoder(env, (jlong) decoder, bounds.width, bounds.height);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeDecode(
  JNIEnv* env, jobject, jlong decoderPtr, jboolean rgb565, jint sampleSize,
  jint x, jint y, jint width, jint height
) {
  auto* decoder = (BaseDecoder*) decoderPtr;

  // Bounds of the image when crop borders is enabled, otherwise it matches the entire image.
  Rect bounds = decoder->info.bounds;

  // Translated requested bounds to the original image.
  Rect inRect = {
    x + bounds.x,
    y + bounds.y,
    (uint32_t) width,
    (uint32_t) height
  };

  // Sampled requested bounds according to sampleSize.
  // It matches the translated bounds when the value is 1
  Rect outRect = inRect.downsample(sampleSize);
  if (outRect.width == 0 || outRect.height == 0) {
    LOGE("Requested sample size too high");
    return nullptr;
  }

  auto* bitmap = create_bitmap(env, outRect.width, outRect.height, rgb565);
  if (!bitmap) {
    LOGE("Failed to create a bitmap of size %dx%dx%d", outRect.width, outRect.height, rgb565 ? 2 : 4);
    return nullptr;
  }

  uint8_t* pixels;
  AndroidBitmap_lockPixels(env, bitmap, (void**) &pixels);
  if (!pixels) {
    LOGE("Failed to lock pixels");
    return nullptr;
  }

  try {
    decoder->decode(pixels, outRect, inRect, rgb565, sampleSize);
  } catch (std::exception &ex) {
    LOGE("%s", ex.what());
    AndroidBitmap_unlockPixels(env, bitmap);
    return nullptr;
  }

  AndroidBitmap_unlockPixels(env, bitmap);
  return bitmap;
}

extern "C"
JNIEXPORT void JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeRecycle(JNIEnv*, jobject, jlong decoderPtr) {
  auto* decoder = (BaseDecoder*) decoderPtr;
  delete decoder;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_tachiyomi_decoder_ImageDecoder_nativeFindType(
  JNIEnv* env, jclass, jbyteArray array
) {
  uint32_t size = env->GetArrayLength(array);

  if (size < 4) {
    LOGW("Not enough bytes to parse info");
    return nullptr;
  }

  auto bytes = (uint8_t*) env->GetByteArrayElements(array, nullptr);

  jobject imageType = nullptr;
  if (JpegDecoder::handles(bytes)) {
    imageType = create_image_type(env, 0, false);
  } else if (PngDecoder::handles(bytes)) {
    imageType = create_image_type(env, 1, false);
  } else if (WebpDecoder::handles(bytes)) {
    try {
      // Create a copy of the bytes, as the decoder would take ownership of the java bytearray
      auto bytesCopy = (uint8_t*) malloc(size); // malloc rather than new because Stream uses C API
      memcpy(bytesCopy, bytes, size);
      auto stream = std::make_unique<Stream>(bytesCopy, size);
      auto decoder = std::make_unique<WebpDecoder>(std::move(stream), false);
      imageType = create_image_type(env, 2, decoder->info.isAnimated);
    } catch (std::exception &ex) {
      LOGW("Failed to parse WebP header. Falling back to non animated WebP");
      imageType = create_image_type(env, 2, false);
    }
  } else if (GifDecoder::handles(bytes)) {
    imageType = create_image_type(env, 3, true);
  } else if (HeifDecoder::handles(bytes)) {
    imageType = create_image_type(env, 4, false);
  } else {
    LOGW("Failed to find image type");
  }

  env->ReleaseByteArrayElements(array, (jbyte*) bytes, 0);

  return imageType;
}
