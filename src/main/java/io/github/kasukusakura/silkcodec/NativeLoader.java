package io.github.kasukusakura.silkcodec;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Properties;

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
        if (f.exists()) {
            try (InputStream is = NativeLoader.class.getResourceAsStream(res);
                 FileInputStream fs = new FileInputStream(f);
            ) {
                if (is == null) throw new InternalError("Resource `" + res + "` not found");
                if (IOKit.isEq(is, fs)) return;
            }
        }
        try (InputStream is = NativeLoader.class.getResourceAsStream(res);
             OutputStream fo = new BufferedOutputStream(new FileOutputStream(f));
        ) {
            if (is == null) throw new InternalError("Resource `" + res + "` not found");
            IOKit.transferTo(is, fo);
        }
    }

    private static void save(Properties p, File f) {
        try (Writer writer = new BufferedWriter(new OutputStreamWriter(
                new FileOutputStream(f), StandardCharsets.UTF_8
        ))) {
            p.store(writer, null);
        } catch (Throwable ignored) {
        }
    }

    private static void init0(File tmpdir) throws Throwable {
        ArrayList<Throwable> ts = new ArrayList<>();
        Properties settings = new Properties();
        if (tmpdir == null) {
            String codecp = System.getProperty("silk-codec.data-path");
            if (codecp != null) {
                tmpdir = new File(codecp);
            }
        }
        if (tmpdir == null) {
            tmpdir = Files.createTempDirectory("silk-codec-kasukusakura").toFile();
        }
        File settingsFile = new File(tmpdir, "settings.properties");
        if (settingsFile.isFile()) {
            try (Reader reader = new BufferedReader(
                    new InputStreamReader(
                            new FileInputStream(settingsFile),
                            StandardCharsets.UTF_8
                    )
            )) {
                settings.load(reader);
            } catch (Throwable throwable) {
                ts.add(throwable);
            }
        }
        {
            String libp = settings.getProperty("native.libp");
            if (libp != null) {
                write(libp, tmpdir, settings.getProperty("native.libn"));
            }
            String nt = settings.getProperty("native.path");
            if (nt != null) {
                System.load(nt);
                return;
            }
        }
        String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
        String arch = System.getProperty("os.arch");
        if (os.startsWith("windows")) {
            String libpath = "natives/windows-shared-x" + (
                    arch.equals("amd64") ? "64" : "86"
            ) + "/silk.dll";
            write(libpath, tmpdir, "silk.dll");
            String pt = new File(tmpdir, "silk.dll").getAbsolutePath();
            System.load(pt);
            settings.setProperty("native.path", pt);
            settings.setProperty("native.libp", libpath);
            settings.setProperty("native.libn", "silk.dll");
            save(settings, settingsFile);
            return;
        }
        if (os.startsWith("mac")) {
            String libpath = "natives/macos/silk.dylib";
            write(libpath, tmpdir, "silk.dylib");
            String pt = new File(tmpdir, "silk.dylib").getAbsolutePath();
            System.load(pt);
            settings.setProperty("native.path", pt);
            settings.setProperty("native.libp", libpath);
            settings.setProperty("native.libn", "silk.dylib");
            save(settings, settingsFile);
            return;
        }
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
                String pt = new File(tmpdir, fname).getAbsolutePath();
                System.load(pt);
                settings.setProperty("native.path", pt);
                settings.setProperty("native.libp", libpath);
                settings.setProperty("native.libn", "silk.so");
                save(settings, settingsFile);
                return;
            } catch (Throwable throwable) {
                ts.add(throwable);
            }
        }
        UnsatisfiedLinkError ule = new UnsatisfiedLinkError("Failed to load native library; @see https://github.com/KasukuSakura/silk-codec/blob/main/BUILD_NATIVE.md");
        for (Throwable t : ts) {
            ule.addSuppressed(t);
        }
        throw ule;
    }
}
