package io.github.kasukusakura.silkcodec;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

class IOKit {
    static void transferTo(InputStream in, OutputStream out) throws IOException {
        transferTo(in, out, false);
    }

    static void transferTo(InputStream in, OutputStream out, boolean flush) throws IOException {
        byte[] buffer = new byte[20480];
        int read;
        while ((read = in.read(buffer)) >= 0) {
            out.write(buffer, 0, read);
            if (flush) {
                out.flush();
            }
        }
    }

    static byte[] sha1(InputStream is) throws NoSuchAlgorithmException, IOException {
        MessageDigest md = MessageDigest.getInstance("SHA1");
        transferTo(is, new OutputStream() {
            @Override
            public void write(int b) throws IOException {
                md.update((byte) b);
            }

            @Override
            public void write(byte @NotNull [] b) throws IOException {
                md.update(b);
            }

            @Override
            public void write(byte @NotNull [] b, int off, int len) throws IOException {
                md.update(b, off, len);
            }
        });
        return md.digest();
    }

    static boolean isEq(InputStream is, InputStream f) throws IOException, NoSuchAlgorithmException {
        return Arrays.equals(sha1(is), sha1(f));
    }
}
