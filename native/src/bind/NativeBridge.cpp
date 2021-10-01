#define NATIVE_NATIVEBRIDGE_H

#include <iostream>
#include "io_github_kasukusakura_silkcodec_NativeBridge.h"
#include "sys_initialize.h"
#include "utils.h"

/*
 * Class:     io_github_kasukusakura_silkcodec_NativeBridge
 * Method:    memcpy
 * Signature: (JJ[B)V
 */
JNIEXPORT void JNICALL Java_io_github_kasukusakura_silkcodec_NativeBridge_memcpy__JJ_3B
        (JNIEnv *env, jclass, jlong addr, jlong size, jbyteArray target) {
    env->SetByteArrayRegion(target, 0, (jsize) size, (jbyte *) (void **) addr);
}

/*
 * Class:     io_github_kasukusakura_silkcodec_NativeBridge
 * Method:    memcpy
 * Signature: ([BIIJ)V
 */
JNIEXPORT void JNICALL Java_io_github_kasukusakura_silkcodec_NativeBridge_memcpy___3BIIJ
        (JNIEnv *env, jclass, jbyteArray src, jint offset, jint size, jlong addr) {
    env->GetByteArrayRegion(src, offset, size, (jbyte *) (void **) addr);
}

/*
 * Class:     io_github_kasukusakura_silkcodec_NativeBridge
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_io_github_kasukusakura_silkcodec_NativeBridge_initialize
        (JNIEnv *env, jclass) {
    sys_initialize_NativeBridge(env);
    sys_initialize_SilkCoder(env);
    sys_initialize_Jvmti(env);
}

static jclass nb_NativeBridge;
static jclass nb_CoderException;
static jfieldID nb_DEB;
static jmethodID nb_fread;
static jmethodID nb_fswrite;

bool isDebug(JNIEnv *env) {
    return env->GetStaticBooleanField(nb_NativeBridge, nb_DEB);
}

void coderException(JNIEnv *env, const char *msg) {
    if (!env->ExceptionCheck()) {
        env->ThrowNew(nb_CoderException, msg);
    }
}

void sys_initialize_NativeBridge(JNIEnv *env) {
    auto NativeBridgeC = env->FindClass("io/github/kasukusakura/silkcodec/NativeBridge");
    nb_NativeBridge = NativeBridgeC;
    if (NativeBridgeC == NULL) return;
    nb_fread = env->GetStaticMethodID(NativeBridgeC, "fread", "(JIJLjava/io/InputStream;)J");
    nb_fswrite = env->GetStaticMethodID(NativeBridgeC, "fswrite", "(JIJLjava/io/OutputStream;)V");
    nb_CoderException = env->FindClass("io/github/kasukusakura/silkcodec/CoderException");
    nb_DEB = env->GetStaticFieldID(NativeBridgeC, "DEB", "Z");
}

// static void fswrite(long buffer, int unit, long length, OutputStream stream)
void nfsfwrite(
        JNIEnv *env,
        void const *_Buffer,
        size_t _ElementSize,
        size_t _ElementCount,
        jobject stream
) {
    env->CallStaticVoidMethod(
            getClass(nb_fswrite),
            nb_fswrite,
            (jlong) _Buffer, (jint) _ElementSize, (jlong) _ElementCount, stream
    );
}

// static long fread(long buffer, int unit, long length, InputStream in)
size_t nfsfread(
        JNIEnv *env,
        void *_Buffer,
        size_t _ElementSize,
        size_t _ElementCount,
        jobject stream
) {
    return (size_t) env->CallStaticLongMethod(
            getClass(nb_fread),
            nb_fread,
            (jlong) _Buffer,
            (jint) _ElementSize,
            (jlong) _ElementCount,
            stream
    );
}
