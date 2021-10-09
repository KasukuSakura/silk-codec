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

    if (debug) {
        std::cout << "[SilkCoder] Encode: tx=" << tencent
                  << ", strict=" << strict
                  << ", fs Hz=" << fs_Hz
                  << ", max sample rate=" << maxInternalSampleRate
                  << ", packet size=" << packetSize
                  << ", band-fec=" << useInBandFEC
                  << ", DTX=" << useDTX
                  << ", complexity=" << complexity
                  << ", bit-rate=" << bitRate
                  << std::endl;
    }

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
            if (debug) {
                std::cout << "[SilkCoder] Writing TX Silk-v3 Header" << std::endl;
            }
            static char Tencent_break[1];
            Tencent_break[0] = (char) 2;
            COS.fwrite(env, Tencent_break, sizeof(char), 1, dst);

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
            COS.fwrite(env, &nBytes_LE, sizeof( SKP_int16 ), 1, bitOutFile );
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
            tencent != 0, strict != 0, debug != 0,
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


/* Seed for the random number generator, which is used for simulating packet loss */
static SKP_int32 rand_seed = 1;


void SilkCoder_decode(
        CrossOperationSystem COS,
        void **env,
        void **source,
        void **dest,
        bool strict,
        bool debug,
        int fs_hz,
        int loss
) {
    unsigned long tottime, starttime;
    size_t counter;
    SKP_int32 totPackets, i, k;
    SKP_int16 ret, len, tot_len;
    SKP_int16 nBytes;
    SKP_uint8 payload[MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES * (MAX_LBRR_DELAY + 1)];
    SKP_uint8 *payloadEnd = NULL, *payloadToDec = NULL;
    SKP_uint8 FECpayload[MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES], *payloadPtr;
    SKP_int16 nBytesFEC;
    SKP_int16 nBytesPerPacket[MAX_LBRR_DELAY + 1], totBytes;
    SKP_int16 out[((FRAME_LENGTH_MS * MAX_API_FS_KHZ) << 1) * MAX_INPUT_FRAMES], *outPtr;
    SKP_int32 packetSize_ms = 0, API_Fs_Hz = 0;
    SKP_int32 decSizeBytes;
    void *psDec;
    SKP_float loss_prob;
    SKP_int32 frames, lost, quiet;
    SKP_SILK_SDK_DecControlStruct DecControl;


    /* default settings */

    loss_prob = 0.0f;


    loss_prob = loss;
    API_Fs_Hz = fs_hz;


    /* Check Silk header */
    {
        char header_buf[50];
        COS.fread(env, header_buf, sizeof(char), 1, source);
        header_buf[strlen("")] = '\0'; /* Terminate with a null character */
        if (strcmp(header_buf, "") != 0) {
            counter = COS.fread(env, header_buf, sizeof(char), strlen("!SILK_V3"), source);
            header_buf[strlen("!SILK_V3")] = '\0'; /* Terminate with a null character */
            if (strcmp(header_buf, "!SILK_V3") != 0) {
                /* Non-equal strings */
                char buf[50];
                snprintf(buf, sizeof(buf), "Error: Wrong Header %s\n", header_buf);
                COS.reportError(env, buf);
                return;
            }
        } else {
            counter = COS.fread(env, header_buf, sizeof(char), strlen("#!SILK_V3"), source);
            header_buf[strlen("#!SILK_V3")] = '\0'; /* Terminate with a null character */
            if (strcmp(header_buf, "#!SILK_V3") != 0) {
                /* Non-equal strings */
                char buf[50];
                snprintf(buf, sizeof(buf), "Error: Wrong Header %s\n", header_buf);
                COS.reportError(env, buf);
                return;
            }
        }
    }

    /* Set the samplingrate that is requested for the output */
    if (API_Fs_Hz == 0) {
        DecControl.API_sampleRate = 24000;
    } else {
        DecControl.API_sampleRate = API_Fs_Hz;
    }

    /* Initialize to one frame per packet, for proper concealment before first packet arrives */
    DecControl.framesPerPacket = 1;

    /* Create decoder */
    ret = SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);
    if (ret) {
        char buf[100];
        snprintf(buf, sizeof(buf), "SKP_Silk_SDK_Get_Decoder_Size returned %d", ret);
        COS.reportError(env, buf);
        return;
    }
    psDec = malloc(decSizeBytes);

    /* Reset decoder */
    ret = SKP_Silk_SDK_InitDecoder(psDec);
    if (ret) {
        char buf[100];
        snprintf(buf, sizeof(buf), "SKP_Silk_InitDecoder returned %d", ret);
        COS.reportError(env, buf);
        return;
    }

    totPackets = 0;
    tottime = 0;
    payloadEnd = payload;

    /* Simulate the jitter buffer holding MAX_FEC_DELAY packets */
    for (i = 0; i < MAX_LBRR_DELAY; i++) {
        /* Read payload size */
        counter = COS.fread(env, &nBytes, sizeof(SKP_int16), 1, source);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( &nBytes, 1 );
#endif
        /* Read payload */
        counter = COS.fread(env, payloadEnd, sizeof(SKP_uint8), nBytes, source);

        if ((SKP_int16) counter < nBytes) {
            break;
        }
        nBytesPerPacket[i] = nBytes;
        payloadEnd += nBytes;
        totPackets++;
    }

    while (1) {
        /* Read payload size */
        counter = COS.fread(env, &nBytes, sizeof(SKP_int16), 1, source);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( &nBytes, 1 );
#endif
        if (nBytes < 0 || counter < 1) {
            break;
        }

        /* Read payload */
        counter = COS.fread(env, payloadEnd, sizeof(SKP_uint8), nBytes, source);
        if ((SKP_int16) counter < nBytes) {
            break;
        }

        /* Simulate losses */
        rand_seed = SKP_RAND(rand_seed);
        if ((((float) ((rand_seed >> 16) + (1 << 15))) / 65535.0f >= (loss_prob / 100.0f)) && (counter > 0)) {
            nBytesPerPacket[MAX_LBRR_DELAY] = nBytes;
            payloadEnd += nBytes;
        } else {
            nBytesPerPacket[MAX_LBRR_DELAY] = 0;
        }

        if (nBytesPerPacket[0] == 0) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for (i = 0; i < MAX_LBRR_DELAY; i++) {
                if (nBytesPerPacket[i + 1] > 0) {
                    starttime = NBS_GetHighResolutionTime();
                    SKP_Silk_SDK_search_for_LBRR(payloadPtr, nBytesPerPacket[i + 1], (i + 1), FECpayload, &nBytesFEC);
                    tottime += NBS_GetHighResolutionTime() - starttime;
                    if (nBytesFEC > 0) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[i + 1];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[0];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr = out;
        tot_len = 0;
        starttime = NBS_GetHighResolutionTime();

        if (lost == 0) {
            /* No Loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);
                if (ret && strict) {
                    char buf[50];
                    snprintf(buf, sizeof(buf), "SKP_Silk_SDK_Decode returned %d", ret);
                    COS.reportError(env, buf);
                    return;
                }

                frames++;
                outPtr += len;
                tot_len += len;
                if (frames > MAX_INPUT_FRAMES) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr = out;
                    tot_len = 0;
                    frames = 0;
                }
                /* Until last 20 ms frame of packet has been decoded */
            } while (DecControl.moreInternalDecoderFrames);
        } else {
            /* Loss: Decode enough frames to cover one packet duration */
            for (i = 0; i < DecControl.framesPerPacket; i++) {
                /* Generate 20 ms */
                ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len);
                if (ret && strict) {
                    char buf[50];
                    snprintf(buf, sizeof(buf), "SKP_Silk_Decode returned %d", ret);
                    COS.reportError(env, buf);
                    return;
                }
                outPtr += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / (DecControl.API_sampleRate / 1000);
        tottime += NBS_GetHighResolutionTime() - starttime;
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( out, tot_len );
#endif
        COS.fwrite(env, out, sizeof(SKP_int16), tot_len, dest);

        /* Update buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }
        /* Check if the received totBytes is valid */
        if (totBytes < 0 || totBytes > sizeof(payload)) {
            char buf[100];
            snprintf(buf, sizeof(buf), "Packets decoded:             %d", totPackets);
            COS.reportError(env, buf);
            return;
        }
        SKP_memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(SKP_uint8));
        payloadEnd -= nBytesPerPacket[0];
        SKP_memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(SKP_int16));

        if (debug && !quiet) {
            fprintf(stderr, "\rPackets decoded:             %d", totPackets);
        }
    }

    /* Empty the recieve buffer */
    for (k = 0; k < MAX_LBRR_DELAY; k++) {
        if (nBytesPerPacket[0] == 0) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for (i = 0; i < MAX_LBRR_DELAY; i++) {
                if (nBytesPerPacket[i + 1] > 0) {
                    starttime = NBS_GetHighResolutionTime();
                    SKP_Silk_SDK_search_for_LBRR(payloadPtr, nBytesPerPacket[i + 1], (i + 1), FECpayload, &nBytesFEC);
                    tottime += NBS_GetHighResolutionTime() - starttime;
                    if (nBytesFEC > 0) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[i + 1];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[0];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr = out;
        tot_len = 0;
        starttime = NBS_GetHighResolutionTime();

        if (lost == 0) {
            /* No loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);
                if (ret && strict) {
                    char buf[50];
                    snprintf(buf, sizeof(buf), "SKP_Silk_SDK_Decode returned %d", ret);
                    COS.reportError(env, buf);
                    return;
                }

                frames++;
                outPtr += len;
                tot_len += len;
                if (frames > MAX_INPUT_FRAMES) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr = out;
                    tot_len = 0;
                    frames = 0;
                }
                /* Until last 20 ms frame of packet has been decoded */
            } while (DecControl.moreInternalDecoderFrames);
        } else {
            /* Loss: Decode enough frames to cover one packet duration */

            /* Generate 20 ms */
            for (i = 0; i < DecControl.framesPerPacket; i++) {
                ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len);
                if (ret && strict) {
                    char buf[50];
                    snprintf(buf, sizeof(buf), "SKP_Silk_Decode returned %d", ret);
                    COS.reportError(env, buf);
                    return;
                }
                outPtr += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / (DecControl.API_sampleRate / 1000);
        tottime += NBS_GetHighResolutionTime() - starttime;
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( out, tot_len );
#endif
        COS.fwrite(env, out, sizeof(SKP_int16), tot_len, dest);

        /* Update Buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }

        /* Check if the received totBytes is valid */
        if (totBytes < 0 || totBytes > sizeof(payload)) {
            if (debug) {
                fprintf(stderr, "\rPackets decoded:              %d", totPackets);
            }
            return;
        }

        SKP_memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(SKP_uint8));
        payloadEnd -= nBytesPerPacket[0];
        SKP_memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(SKP_int16));

        if (debug && !quiet) {
            fprintf(stderr, "\rPackets decoded:              %d", totPackets);
        }
    }

    if (debug && !quiet) {
        printf("\nDecoding Finished \n");
    }

    /* Free decoder */
    free(psDec);

    /* Close files */
//    fclose(speechOutFile);
//    fclose(bitInFile);

    if (debug) {
        auto filetime = totPackets * 1e-3 * packetSize_ms;
        fprintf(stderr, "\nFile length:                 %.3f s", filetime);
        fprintf(stderr, "\nTime for decoding:           %.3f s (%.3f%% of realtime)",
                1e-6 * tottime,
                1e-4 * tottime / filetime
        );
        fprintf(stderr, "\n\n");
    }

}

/*
 * Class:     io_github_kasukusakura_silkcodec_SilkCoder
 * Method:    decode
 * Signature: (Ljava/io/InputStream;Ljava/io/OutputStream;ZII)V
 */
JNIEXPORT void JNICALL Java_io_github_kasukusakura_silkcodec_SilkCoder_decode(
        JNIEnv *env, jclass,
        jobject src, jobject dst,
        jboolean strict, jint fs_Hz,
        jint loss
) {
    if (requireNonNull(env, src, "src")) return;
    if (requireNonNull(env, dst, "dst")) return;
    if (fs_Hz == 0) {
        coderException(env, "fs_Hz: 0");
        return;
    }
    auto debug = isDebug(env);

    SilkCoder_decode(
            JVM_IO_SYSTEM(),
            (void **) env,
            (void **) src,
            (void **) dst,
            (bool) strict,
            debug,
            (int) fs_Hz,
            (int) loss
    );
}
