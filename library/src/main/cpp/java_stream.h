//
// Created by len on 23/12/20.
//

#ifndef IMAGEDECODER_JAVA_STREAM_H
#define IMAGEDECODER_JAVA_STREAM_H

#include <stdlib.h>
#include <memory>
#include <jni.h>
#include "log.h"
#include "stream.h"

void init_java_stream(JNIEnv* env);

std::unique_ptr<Stream> read_all_java_stream(JNIEnv* env, jobject jstream);

#endif //IMAGEDECODER_JAVA_STREAM_H
