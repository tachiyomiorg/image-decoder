//
// Created by len on 23/12/20.
//

#include "java_objects.h"

static jclass imageDecoderCls;
static jmethodID imageDecoderCtor;
static jmethodID createBitmapMethod;

void init_java_objects(JNIEnv* env) {
  jclass tmpDecoderCls = env->FindClass("tachiyomi/decoder/ImageDecoder");
  imageDecoderCls = (jclass) env->NewGlobalRef(tmpDecoderCls);
  imageDecoderCtor = env->GetMethodID(imageDecoderCls, "<init>", "(JII)V");
  createBitmapMethod = env->GetStaticMethodID(imageDecoderCls, "createBitmap", "(IIZ)Landroid/graphics/Bitmap;");
  env->DeleteLocalRef(tmpDecoderCls);
}

jobject create_image_decoder(JNIEnv* env, jlong decoderPtr, jint width, jint height) {
  return env->NewObject(imageDecoderCls, imageDecoderCtor, decoderPtr, width, height);
}

jobject create_bitmap(JNIEnv* env, jint width, jint height, jboolean rgb565) {
  return env->CallStaticObjectMethod(imageDecoderCls, createBitmapMethod, width, height, rgb565);
}
