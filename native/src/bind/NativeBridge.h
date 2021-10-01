#ifndef NATIVE_NATIVEBRIDGE_H
#define NATIVE_NATIVEBRIDGE_H

#include "../../jni_include/common/jni.h"

// io/github/kasukusakura/silkcodec/NativeBridge
extern jclass nb_NativeBridge;

// static long fread(long buffer, int unit, long length, InputStream in)
extern jmethodID nb_fread;

// static void fswrite(long buffer, int unit, long length, OutputStream stream)
extern jmethodID nb_fswrite;


void nfsfwrite(
        JNIEnv *,
        void const *_Buffer,
        size_t _ElementSize,
        size_t _ElementCount,
        jobject stream
);

size_t nfsfread(
        JNIEnv *,
        void *_Buffer,
        size_t _ElementSize,
        size_t _ElementCount,
        jobject stream
);

void coderException(JNIEnv *, const char *msg);

bool isDebug(JNIEnv *);

#endif //NATIVE_NATIVEBRIDGE_H
