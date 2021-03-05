//
// Created by len on 23/12/20.
//

#include "java_stream.h"

#define BUFFER_SIZE 8192
#define CONTAINER_DEFAULT_SIZE (BUFFER_SIZE * 50)

static jmethodID readMethod;
static jmethodID availableMethod;

void init_java_stream(JNIEnv* env) {
  jclass streamCls = env->FindClass("java/io/InputStream");
  readMethod = env->GetMethodID(streamCls, "read", "([BII)I");
  availableMethod = env->GetMethodID(streamCls, "available", "()I");
  env->DeleteLocalRef(streamCls);
}

std::unique_ptr<Stream> read_all_java_stream(JNIEnv* env, jobject jstream) {
  jbyteArray buffer;
  uint8_t* stream = nullptr;

  int available = env->CallIntMethod(jstream, availableMethod);
  uint32_t streamReservedSize = available > BUFFER_SIZE ? available : CONTAINER_DEFAULT_SIZE;
  uint32_t streamOffset = 0;

  // Make sure the stream didn't throw an exception before env calls
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    goto fail;
  }

  buffer = env->NewByteArray(BUFFER_SIZE);
  if (!buffer) {
    goto fail;
  }

  // Use malloc to make it compatible with realloc and C++ unique_ptr with custom deleter
  stream = (uint8_t*) malloc(streamReservedSize);
  if (!stream) {
    goto fail;
  }

  int read;
  while (true) {
    read = env->CallIntMethod(jstream, readMethod, buffer, 0, BUFFER_SIZE);
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      goto fail;
    }
    if (read < 0) {
      break;
    }
    while (streamReservedSize < streamOffset + read) {
      streamReservedSize = (int) (streamReservedSize * 1.5);
      auto* tmp = (uint8_t*) realloc(stream, streamReservedSize);
      if (!tmp) {
        goto fail;
      }
      stream = tmp;
    }

    auto* dest = reinterpret_cast<jbyte*>(stream + streamOffset);
    env->GetByteArrayRegion(buffer, 0, read, dest);

    streamOffset += read;
  }

  if (streamOffset == 0) {
    goto fail;
  }

  return std::make_unique<Stream>(stream, streamOffset);

fail:
  free(stream);
  return nullptr;
}
