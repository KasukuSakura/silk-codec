package io.github.kasukusakura.silkcodec;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SilkCoder {
    static {
        NativeBridge.initialize_pub();
    }

    /**
     * Encode PCM (pcm_s16le) to Silk v3
     */
    public static native void encode(
            InputStream source,
            OutputStream dest,
            boolean tencent,
            boolean strict,

            int fs_Hz,
            int maxInternalSampleRate,
            int packetSize,
            int packetLossPercentage,
            int useInBandFEC,
            int useDTX,
            int complexity,
            int bitRate
    ) throws IOException;

    public static void encode(InputStream source,
                              OutputStream dest,
                              int sampleRate,
                              int bitRate
    ) throws IOException {
        encode(source, dest, sampleRate, bitRate, true, true);
    }

    public static void encode(InputStream source,
                              OutputStream dest,
                              int sampleRate,
                              int bitRate,
                              boolean tencent,
                              boolean strict
    ) throws IOException {
        encode(source,
                dest, tencent, strict,
                sampleRate,
                24000,
                (20 * sampleRate / 1000),
                0,
                0,
                0,
                2,
                bitRate
        );
    }

    public static void encode(InputStream source,
                              OutputStream dest,
                              int sampleRate
    ) throws IOException {
        encode(source, dest, sampleRate, 24000);
    }

    /**
     * Decode SilkV3 to PCM (pcm_s16le)
     */
    public static native void decode(
            InputStream src, OutputStream dst,
            boolean strict,
            int simpleRate,
            int loss
    ) throws IOException;

    public static void decode(
            InputStream src, OutputStream dst
    ) throws IOException {
        decode(src, dst, true, 24000, 0);
    }
}
