#include "utils.h"
#include "jvmti.h"
#include <memory>

static jvmtiEnv *S_JVMTI;

bool requireNonNull(JNIEnv *env, jobject o, const char *msg) {
    if (o == NULL) {
        env->ThrowNew(env->FindClass("java/lang/NullPointerException"), msg);
        return true;
    }
    return false;
}

void sys_initialize_Jvmti(JNIEnv *env) {
    JavaVM *vm;
    env->GetJavaVM(&vm);

    jint jnierror = vm->GetEnv((void **) &S_JVMTI,
                               JVMTI_VERSION_1_2);
    if (jnierror != 0) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "Jvmti initialize failed");
        return;
    }
}

jclass getClass(jmethodID jmid) {
    jclass rsp;
    S_JVMTI->GetMethodDeclaringClass(jmid, &rsp);
    return rsp;
}
