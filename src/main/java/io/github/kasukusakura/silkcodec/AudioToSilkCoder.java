package io.github.kasukusakura.silkcodec;

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.Executor;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AudioToSilkCoder {
    private final Executor threadPool;
    private final int ffmpegBufferSize;

    public AudioToSilkCoder(Executor threadPool) {
        this(threadPool, 204800);
    }

    public AudioToSilkCoder(Executor threadPool, int ffmpegBufferSize) {
        this.threadPool = Objects.requireNonNull(threadPool, "threadPool");
        this.ffmpegBufferSize = ffmpegBufferSize;
    }

    public void connect(String ffmpeg, String srcFile, OutputStream output) throws IOException {
        connect(new ProcessBuilder(ffmpegC(ffmpeg, srcFile)).start(), null, output);
    }

    public void connect(Process process, InputStream stdin, OutputStream output) throws IOException {
        try {
            if (stdin != null) {
                threadPool.execute(() -> {
                    try {
                        IOKit.transferTo(stdin, process.getOutputStream(), true);
                        process.getOutputStream().close();
                        // stdin.close();
                        if (NativeBridge.DEB) {
                            System.out.println("[AudioToSilkCoder] stdin completed");
                        }
                    } catch (Throwable throwable) {
                        errorLog(throwable);
                    }
                });
            }
            PipedInputStream processIn = new PipedInputStream(ffmpegBufferSize);
            PipedOutputStream $$tm_pin_trans = new PipedOutputStream(processIn);

            threadPool.execute(() -> {
                try {
                    IOKit.transferTo(process.getInputStream(), $$tm_pin_trans, true);
                    if (NativeBridge.DEB) {
                        System.out.println("[AudioToSilkCoder] pcm completed");
                    }
                    $$tm_pin_trans.close();
                } catch (Throwable throwable) {
                    errorLog(throwable);
                }
            });


            BufferedReader bf = new BufferedReader(new InputStreamReader(process.getErrorStream()));
            while (true) {
                String nextLine = bf.readLine();
                if (nextLine == null) break;
                cmdLine(nextLine);
                if (nextLine.startsWith("Output ")) break;
            }
            int hz = 0;
            Pattern pt = Pattern.compile("(\\d+) Hz");
            while (true) {
                String nextLine = bf.readLine();
                if (nextLine == null) break;
                cmdLine(nextLine);
                if (nextLine.trim().startsWith("Stream ")) {
                    Matcher matcher = pt.matcher(nextLine);
                    if (matcher.find()) {
                        hz = Integer.parseInt(matcher.group(1));
                        cmdLine("###HZ: " + hz);
                        break;
                    }
                }
            }
            if (hz == 0) {
                throw new IOException("Cannot find Hz");
            }
            if (hz == 22050) {
                if (!Boolean.getBoolean("silk-codec.rate-check-ignore")) {
                    throw new IllegalArgumentException("22050 Hz is not supported. If you need to ignore this check, please set `silk-codec.rate-check-ignore=true`");
                }
            }
            threadPool.execute(() -> {
                try {
                    // drop error input
                    while (true) {
                        String nl = bf.readLine();
                        if (nl == null) break;
                        cmdLine(nl);
                    }
                } catch (Throwable throwable) {
                    errorLog(throwable);
                }
            });
            try {
                SilkCoder.encode(processIn, output, hz);
                if (NativeBridge.DEB) {
                    System.out.println("[P-INT] shutdown");
                }
            } catch (CoderException exception) {
                throw new IOException("Cannot encode Hz: " + hz, exception);
            }
        } catch (Throwable throwable) {
            try {
                process.destroy();
            } catch (Throwable throwable1) {
                throwable.addSuppressed(throwable1);
            }
            if (stdin != null) {
                try {
                    stdin.close();
                } catch (Throwable ioe) {
                    throwable.addSuppressed(ioe);
                }
            }
            throw throwable;
        }
    }

    public void connect(String ffmpeg, InputStream src, OutputStream output) throws IOException {
        connect(new ProcessBuilder(ffmpegC(ffmpeg, null)).start(),
                src,
                output
        );
    }

    protected void cmdLine(String nextLine) {
        if (NativeBridge.DEB) {
            System.out.println(nextLine);
        }
    }

    protected void errorLog(Throwable throwable) {
        throwable.printStackTrace();
    }

    private static List<String> ffmpegC(String ffmpeg, String file) {
        ArrayList<String> cmd = new ArrayList<>();
        cmd.add(ffmpeg);
        cmd.add("-y");
        cmd.add("-i");
        cmd.add(file == null ? "-" : file);
        cmd.add("-acodec");
        cmd.add("pcm_s16le");
        cmd.add("-f");
        cmd.add("s16le");
        cmd.add("-ac");
        cmd.add("1");
        cmd.add("-");
        return cmd;
    }

}
