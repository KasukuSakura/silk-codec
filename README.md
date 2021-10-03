# silk-codec

A library for convert PCM to tencent silk files.

## Features

- [X] Convert PCM to silk
- [X] Convert audio to silk (FFMPEG required)
- [X] Convert silk to PCM

## Platform supported

- Windows x64
- Windows x86
- Linux x64
- Linux arm64
- Android x86
- Android x86_64
- Android arm64
- Mac OS

## Usage

### CLI

```shell
ffmpeg -y -i $INPUT -acodec pcm_s16le -f s16le -ac 1 tmp.pcm
silk_codec pts -i tmp.pcm -s $HZ -o out.silk
```

```text
ffmpeg version git-2020-08-16-5df9724 Copyright (c) 2000-2020 the FFmpeg developers
Input #0, mp3, from 'Conqueror.mp3':
Stream mapping:
  Stream #0:0 -> #0:0 (mp3 (mp3float) -> pcm_s16le (native))
Press [q] to stop, [?] for help
Output #0, s16le, to 'pipe:':
  Metadata:
    Stream #0:0: Audio: pcm_s16le, 44100 Hz, mono, s16, 705 kb/s
                                   ^~~~~
                                   It is $HZ
```

### Java

Jar was published as `io.github.kasukusakura:silk-codec`

```java
public class A {
    public static void normal() throws Throwable {
        // System.load(); // load native if necessary
        var simpleRate = 44100;
        var pcm = "src.pcm";

        try (var som = new BufferedOutputStream(new FileOutputStream(
                "out.silk"
        ))) {
            SilkCoder.encode(
                    new BufferedInputStream(new FileInputStream(pcm)),
                    som,
                    simpleRate
            );
        }
    }

    public static void any() throws Throwable {
        // System.load(); // load native if necessary

        var threadPool = Executors.newCachedThreadPool();

        var stream = new AudioToSilkCoder(threadPool);
        try (var fso = new BufferedOutputStream(new FileOutputStream("out.silk"))) {
            stream.connect("ffmpeg", "src.mp3", fso);
        }
        System.out.println("DONE");
        threadPool.shutdown();
    }
}
```
