#ifndef NATIVE_SILKCODER_H
#define NATIVE_SILKCODER_H

#include "CrossOperationSystem.h"

void SilkerCoder_encode(
        CrossOperationSystem COS,
        void **env, void **src, void **dst,

        bool tencent, bool strict, bool debug,

        int fs_Hz,
        int maxInternalSampleRate,
        int packetSize,
        int packetLossPercentage,
        int useInBandFEC,
        int useDTX,
        int complexity,
        int bitRate
);

#endif //NATIVE_SILKCODER_H
