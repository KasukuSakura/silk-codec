package io.github.kasukusakura.silkcodec;

import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class WrapDmpSave extends FilterInputStream {
    private OutputStream os;

    public WrapDmpSave(InputStream delegate, OutputStream os) {
        super(delegate);
        this.os = os;
    }

    @Override
    public int read() throws IOException {
        int i = in.read();
        if (i != -1) os.write(i);

        return i;
    }

    @Override
    public int read(byte[] b) throws IOException {
        int rsp = in.read(b);

        if (rsp != -1) os.write(b, 0, rsp);

        return rsp;
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        int rsp = in.read(b, off, len);
        // System.out.println("[WDS] RD " + rsp + "<==" + off + " <> " + len);

        if (rsp != -1) os.write(b, off, rsp);
        else if (os != null) {
            os.close();
            os = null;
        }

        return rsp;
    }

    @Override
    public void close() throws IOException {
        super.close();
        if (os != null) os.close();
    }

    @Override
    public String toString() {
        return "WDS{in=" + in.getClass() + ", os=" + os.getClass() + "}";
    }
}
