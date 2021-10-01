package io.github.kasukusakura.silkcodec;

import java.io.InputStream;
import java.io.OutputStream;

class IOKit {
    static void transferTo(InputStream in, OutputStream out) throws Throwable {
        transferTo(in, out, false);
    }

    static void transferTo(InputStream in, OutputStream out, boolean flush) throws Throwable {
        byte[] buffer = new byte[20480];
        int read;
        while ((read = in.read(buffer)) >= 0) {
            out.write(buffer, 0, read);
            if (flush) {
                out.flush();
            }
        }
    }
}
