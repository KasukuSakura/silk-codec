#ifndef NATIVE_CROSSOPERATIONSYSTEM_H
#define NATIVE_CROSSOPERATIONSYSTEM_H

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    void (*fwrite)(
            void **,
            void const *_Buffer,
            size_t _ElementSize,
            size_t _ElementCount,
            void **output
    );

    size_t (*fread)(
            void **,
            void *_Buffer,
            size_t _ElementSize,
            size_t _ElementCount,
            void **input
    );

    void (*reportError)(
            void **,
            const char *
    );

    bool (*ExceptionCheck)(void **);
} CrossOperationSystem;

CrossOperationSystem NATIVE_FILE_SYSTEM();
CrossOperationSystem JVM_IO_SYSTEM();

#ifdef __cplusplus
}
#endif

#endif //NATIVE_CROSSOPERATIONSYSTEM_H
