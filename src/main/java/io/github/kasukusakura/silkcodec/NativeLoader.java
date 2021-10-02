package io.github.kasukusakura.silkcodec;

import java.io.*;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Locale;

public class NativeLoader {
    private static boolean initializing = false;

    public static synchronized void initialize(File tmpdir) {
        if (initializing) return;
        try {
            initializing = true;
            NativeBridge.noop();
        } catch (UnsatisfiedLinkError e) {
            try {
                init0(tmpdir);
            } catch (Throwable throwable) {
                initializing = false;
                throw (UnsatisfiedLinkError) new UnsatisfiedLinkError().initCause(throwable);
            }
        }
        NativeBridge.initialize();
        initializing = false;
    }

    private static void write(String res, File f, String name) throws Throwable {
        f = new File(f, name);
        if (f.exists()) return;
        try (InputStream is = NativeLoader.class.getResourceAsStream(res);
             OutputStream fo = new BufferedOutputStream(new FileOutputStream(f));
        ) {
            if (is == null) throw new InternalError("Resource `" + res + "` not found");
            IOKit.transferTo(is, fo);
        }
    }

    private static void init0(File tmpdir) throws Throwable {
        if (tmpdir == null) {
            tmpdir = Files.createTempDirectory("silk-codec-kasukusakura").toFile();
        }
        String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
        String arch = System.getProperty("os.arch");
        if (os.startsWith("windows")) {
            String libpath = "natives/windows-shared-x" + (
                    arch.equals("amd64") ? "64" : "86"
            ) + "/silk.dll";
            write(libpath, tmpdir, "silk.dll");
            System.load(new File(tmpdir, "silk.dll").getAbsolutePath());
            return;
        }
        if (os.startsWith("mac")) {
            String libpath = "natives/macos/silk.dylib";
            write(libpath, tmpdir, "silk.dylib");
            System.load(new File(tmpdir, "silk.dylib").getAbsolutePath());
            return;
        }
        ArrayList<Throwable> ts = new ArrayList<>();
        for (String platform : new String[]{
                "linux-x64",
                "linux-arm64",
                "android-x86",
                "android-x86_64",
                "android-arm64",
        }) {
            String libpath = "natives/" + platform + "/silk.so";
            String fname = "silk-" + platform + ".so";
            write(libpath, tmpdir, fname);
            try {
                System.load(new File(tmpdir, fname).getAbsolutePath());
            } catch (Throwable throwable) {
                ts.add(throwable);
            }
            return;
        }
        UnsatisfiedLinkError ule = new UnsatisfiedLinkError("Failed to load native library");
        for (Throwable t : ts) {
            ule.addSuppressed(t);
        }
        throw ule;
    }
}
