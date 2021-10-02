package ci;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

@SuppressWarnings("ConstantConditions")
public class PNative {
    private static void cp(File s, File t) throws IOException {
        if (!s.isFile()) return;
        t.getParentFile().mkdirs();
        Files.copy(s.toPath(), t.toPath(), StandardCopyOption.REPLACE_EXISTING);
    }

    public static void main(String[] args) throws Throwable {
        var artifacts = new File("tmp/artifacts");
        var rs = new File("src/main/resources/io/github/kasukusakura/silkcodec/natives");
        rs.mkdirs();
        var exec = new File("tmp/exec");
        for (var platform : artifacts.listFiles()) {
            if (platform.isFile()) continue;
            for (var nativeLib : new String[]{
                    "silk",
            }) {
                for (var dllTypes : new String[]{
                        "dll", "so", "dylib"
                }) {
                    cp(
                            new File(platform, nativeLib + "." + dllTypes),
                            new File(rs, platform.getName() + "/" + nativeLib + "." + dllTypes)
                    );
                }
            }
            for (var execs : new String[]{
                    "pcm_to_silk"
            }) {
                cp(
                        new File(platform, execs),
                        new File(exec, execs + "-" + platform.getName())
                );
                cp(
                        new File(platform, execs + ".exe"),
                        new File(exec, execs + "-" + platform.getName() + ".exe")
                );
            }
        }
    }
}
