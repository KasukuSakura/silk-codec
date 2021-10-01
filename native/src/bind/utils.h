#ifndef NATIVE_UTILS_H
#define NATIVE_UTILS_H

#include "../../jni_include/common/jni.h"

bool requireNonNull(JNIEnv *, jobject, const char *msg);

jclass getClass(jmethodID);

#endif //NATIVE_UTILS_H
