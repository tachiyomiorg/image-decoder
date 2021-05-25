//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_JAVA_OBJECTS_H
#define IMAGEDECODER_JAVA_OBJECTS_H

#include <jni.h>

void init_java_objects(JNIEnv* env);

jobject create_image_decoder(JNIEnv* env, jlong decoderPtr, jint width, jint height);

jobject create_bitmap(JNIEnv* env, jint width, jint height, jboolean rgb565);

jobject create_image_type(JNIEnv* env, jint format, jboolean isAnimated);

#endif //IMAGEDECODER_JAVA_OBJECTS_H
