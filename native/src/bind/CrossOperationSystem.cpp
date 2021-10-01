#include "CrossOperationSystem.h"
#include "jni.h"
#include "NativeBridge.h"

bool JExceptionCheck(JNIEnv *env) {
    return env->ExceptionCheck();
}

CrossOperationSystem COS_JVM_IO_SYSTEM = {
        reinterpret_cast<void (*)(void **, const void *, size_t, size_t, void **)>(&nfsfwrite),
        reinterpret_cast<size_t (*)(void **, void *, size_t, size_t, void **)>(&nfsfread),
        reinterpret_cast<void (*)(void **, const char *)>(&coderException),
        reinterpret_cast<bool (*)(void **)>(&JExceptionCheck),
};

CrossOperationSystem COS_NATIVE_FILE_SYSTEM = {
        [](auto, auto _Buffer, auto _ElementSize, auto _ElementCount, auto output) {
            fwrite(_Buffer, _ElementSize, _ElementCount, (FILE *) output);
        },
        [](auto, auto _Buffer, auto _ElementSize, auto _ElementCount, auto input) {
            return fread(_Buffer, _ElementSize, _ElementCount, (FILE *) input);
        },
        [](void **, auto msg) {
            std::cerr << msg << std::endl;
        },
        [](void **) { return false; },
};

CrossOperationSystem NATIVE_FILE_SYSTEM() {
    return COS_NATIVE_FILE_SYSTEM;
}

CrossOperationSystem JVM_IO_SYSTEM() {
    return COS_JVM_IO_SYSTEM;
}
