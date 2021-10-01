#include "io_github_kasukusakura_silkcodec_SilkCoder.h"
#include "SilkCoder.h"
#include "coder.h"
#include "utils.h"
#include "NativeBridge.h"
#include <exception>
#include <stdexcept>
#include <iostream>

void sys_initialize_SilkCoder(JNIEnv *) {}

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
) {

    unsigned long tottime, starttime;
    double filetime;
    size_t counter;
    SKP_int32 k, args, totPackets, totActPackets, ret;
    SKP_int16 nBytes;
    double sumBytes, sumActBytes, avg_rate, act_rate, nrg;
    SKP_uint8 payload[MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES];
    SKP_int16 in[FRAME_LENGTH_MS * MAX_API_FS_KHZ * MAX_INPUT_FRAMES];
    SKP_int32 encSizeBytes;
    void *psEnc;
#ifdef _SYSTEM_IS_BIG_ENDIAN
    SKP_int16 nBytes_LE;
#endif

    /* default settings */
    SKP_int32 API_fs_Hz = fs_Hz;
    SKP_int32 max_internal_fs_Hz = 0;
    SKP_int32 targetRate_bps = 25000;
    SKP_int32 smplsSinceLastPacket, packetSize_ms = 20;
    SKP_int32 frameSizeReadFromFile_ms = 20;
    SKP_int32 packetLoss_perc = 0;

#if LOW_COMPLEXITY_ONLY
    SKP_int32 complexity_mode = 0;
#else
    SKP_int32 complexity_mode = 2;
#endif

    SKP_int32 DTX_enabled = 0, INBandFEC_enabled = 0, quiet = 0;
    SKP_SILK_SDK_EncControlStruct encControl; // Struct for input to encoder
    SKP_SILK_SDK_EncControlStruct encStatus;  // Struct for status of encoder

    /* Add Silk header to stream */
    {
        if (tencent) {
            static char Tencent_break[1];
            Tencent_break[0] = (char) 2;
            COS.fwrite(env, Tencent_break, sizeof(char), strlen(Tencent_break), dst);

            if (COS.ExceptionCheck(env)) return;
        }

        static const char Silk_header[] = "#!SILK_V3";
        COS.fwrite(env, Silk_header, sizeof(char), strlen(Silk_header), dst);

        if (COS.ExceptionCheck(env)) return;
    }

    /* Create Encoder */
    ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
    if (ret) {
        char buff[50];
        snprintf(buff, sizeof(buff), "Error: SKP_Silk_create_encoder returned %d", ret);
        COS.reportError(env, buff);
        return;
    }

    psEnc = malloc(encSizeBytes);

    /* Reset Encoder */
    ret = SKP_Silk_SDK_InitEncoder(psEnc, &encStatus);
    if (ret) {
        char buff[100];
        snprintf(buff, sizeof(buff),
                 "Error: SKP_Silk_reset_encoder returned %d",
                 ret
        );
        COS.reportError(env, buff);
        return;
    }

    /* Set Encoder parameters */
    encControl.API_sampleRate = API_fs_Hz;
    encControl.maxInternalSampleRate = maxInternalSampleRate;
    encControl.packetSize = packetSize;
    encControl.packetLossPercentage = packetLossPercentage;
    encControl.useInBandFEC = useInBandFEC;
    encControl.useDTX = useDTX;
    encControl.complexity = complexity;
    encControl.bitRate = (bitRate > 0 ? bitRate : 0);

    if (API_fs_Hz > MAX_API_FS_KHZ * 1000 || API_fs_Hz < 0) {
        char buff[100];
        snprintf(buff, sizeof(buff),
                 "Error: API sampling rate = %d out of range, valid range 8000 - 48000",
                 API_fs_Hz
        );
        COS.reportError(env, buff);
        return;
    }

    tottime = 0;
    totPackets = 0;
    totActPackets = 0;
    smplsSinceLastPacket = 0;
    sumBytes = 0.0;
    sumActBytes = 0.0;
    smplsSinceLastPacket = 0;
    if (debug) {
        std::cout << "[SilkCoder] Start...." << std::endl;
    }
    while (1) {
        /* Read input from file */
        counter = COS.fread(env, in, sizeof(SKP_int16), (frameSizeReadFromFile_ms * API_fs_Hz) / 1000, src);
        if (COS.ExceptionCheck(env)) return;
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( in, counter );
#endif
        if (((SKP_int) counter) < ((frameSizeReadFromFile_ms * API_fs_Hz) / 1000)) {
            break;
        }

        /* max payload size */
        nBytes = MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES;

        // starttime = NBS_GetHighResolutionTime();

        /* Silk Encoder */
        ret = SKP_Silk_SDK_Encode(psEnc, &encControl, in, (SKP_int16) counter, payload, &nBytes);
        if (ret != 0 && strict) {
            char buff[50];
            snprintf(buff, sizeof(buff), "SKP_Silk_Encode returned %d", ret);
            COS.reportError(env, buff);
            return;
        }

        // tottime += NBS_GetHighResolutionTime() - starttime;

        /* Get packet size */
        packetSize_ms = (SKP_int) ((1000 * (SKP_int32) encControl.packetSize) / encControl.API_sampleRate);

        smplsSinceLastPacket += (SKP_int) counter;

        if (((1000 * smplsSinceLastPacket) / API_fs_Hz) == packetSize_ms) {
            /* Sends a dummy zero size packet in case of DTX period  */
            /* to make it work with the decoder test program.        */
            /* In practice should be handled by RTP sequence numbers */
            totPackets++;
            sumBytes += nBytes;
            nrg = 0.0;
            for (k = 0; k < (SKP_int) counter; k++) {
                nrg += in[k] * (double) in[k];
            }
            if ((nrg / (SKP_int) counter) > 1e3) {
                sumActBytes += nBytes;
                totActPackets++;
            }

            /* Write payload size */
#ifdef _SYSTEM_IS_BIG_ENDIAN
            nBytes_LE = nBytes;
            swap_endian( &nBytes_LE, 1 );
            nfsfwrite( &nBytes_LE, sizeof( SKP_int16 ), 1, bitOutFile );
#else
            COS.fwrite(env, &nBytes, sizeof(SKP_int16), 1, dst);
#endif
            if (COS.ExceptionCheck(env)) return;

            /* Write payload */
            COS.fwrite(env, payload, sizeof(SKP_uint8), nBytes, dst);
            if (COS.ExceptionCheck(env)) return;

            smplsSinceLastPacket = 0;

//            if (!quiet) {
//                fprintf(stderr, "\rPackets encoded:                %d", totPackets);
//            }
        }
    }

    /* Write dummy because it can not end with 0 bytes */
    nBytes = -1;

    /* Write payload size */
    if (!tencent) {
        if (debug) {
            std::cout << "[SilkCoder] Writing payload size" << std::endl;
        }
        COS.fwrite(env, &nBytes, sizeof(SKP_int16), 1, dst);
    }

    /* Free Encoder */
    free(psEnc);

    if (debug) {
        std::cout << "[SilkCoder] Encode Done" << std::endl;
    }
    /*
    filetime = totPackets * 1e-3 * packetSize_ms;
    avg_rate = 8.0 / packetSize_ms * sumBytes / totPackets;
    act_rate = 8.0 / packetSize_ms * sumActBytes / totActPackets;

    if (!quiet) {
        printf("\nFile length:                    %.3f s", filetime);
        printf("\nTime for encoding:              %.3f s (%.3f%% of realtime)", 1e-6 * tottime,
               1e-4 * tottime / filetime);
        printf("\nAverage bitrate:                %.3f kbps", avg_rate);
        printf("\nActive bitrate:                 %.3f kbps", act_rate);
        printf("\n\n");
    } else {
        // print time and % of realtime
        printf("%.3f %.3f %d ", 1e-6 * tottime, 1e-4 * tottime / filetime, totPackets);
        // print average and active bitrates
        printf("%.3f %.3f \n", avg_rate, act_rate);
    }
    */
}


JNIEXPORT void JNICALL Java_io_github_kasukusakura_silkcodec_SilkCoder_encode(
        JNIEnv *env, jclass,
        jobject src, jobject dst,
        jboolean tencent, jboolean strict,
        jint fs_Hz,
        jint maxInternalSampleRate,
        jint packetSize,
        jint packetLossPercentage,
        jint useInBandFEC,
        jint useDTX,
        jint complexity,
        jint bitRate
) {
    if (requireNonNull(env, src, "src")) return;
    if (requireNonNull(env, dst, "dst")) return;
    if (fs_Hz == 0) {
        coderException(env, "fs_Hz: 0");
        return;
    }
    auto debug = isDebug(env);

    SilkerCoder_encode(
            JVM_IO_SYSTEM(),
            reinterpret_cast<void **>(env),
            reinterpret_cast<void **>(src),
            reinterpret_cast<void **>(dst),
            tencent, strict, debug,
            fs_Hz,
            maxInternalSampleRate,
            packetSize,
            packetLossPercentage,
            useInBandFEC,
            useDTX,
            complexity,
            bitRate
    );
}
