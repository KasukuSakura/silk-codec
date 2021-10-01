package io.github.kasukusakura.silkcodec;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

@SuppressWarnings({"UnusedReturnValue", "SameParameterValue"})
class NativeBridge {
    static boolean DEB = false;
    static Throwable it;

    private static final ThreadLocal<byte[]> TMPBUF = ThreadLocal.withInitial(() -> new byte[81920]);

    static native void memcpy(long address, long size, byte[] target);

    static native void memcpy(byte[] src, int offset, int size, long address);

    static native void noop();

    static native void initialize();

    static void initialize_pub() {
    }

    static {
        NativeLoader.initialize(null);
    }

    static long fread(long buffer, int unit, long length, InputStream in) throws IOException {
        long rsp = 0;
        byte[] buf = TMPBUF.get();

        length *= unit;
        // Unsafe usf = Unsafe.getUnsafe();

        while (length > 0) {
            int rd, lnint = (int) length;
            if (lnint < 1) {
                rd = buf.length;
            } else {
                rd = Math.min(buf.length, lnint);
            }
            int size = in.read(buf, 0, rd);
            if (size == -1) break;

            // usf.copyMemory(buf, Unsafe.ARRAY_BYTE_BASE_OFFSET, null, buffer, size);
            memcpy(buf, 0, size, buffer);

            buffer += size;
            rsp += size;
            length -= size;
        }
        return rsp / unit;
    }

    static void fswrite(long buffer, int unit, long length, OutputStream stream) throws IOException {
        length *= unit;

        byte[] bf = TMPBUF.get();
        while (length > 0) {
            int lnint = (int) length, s;
            if (lnint < 1) {
                s = bf.length;
            } else {
                s = Math.min(bf.length, lnint);
            }

            memcpy(buffer, s, bf);
            buffer += s;
            length -= s;
            stream.write(bf, 0, s);
        }
    }
}
