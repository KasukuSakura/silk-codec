package ci;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

@SuppressWarnings("ConstantConditions")
public class UploadArtifacts {
    public static void main(String[] args) throws Throwable {
        // osname
        //    `- library.dll
        // Jars
        //    `- ....
        String version = args[0];
        String postUrl = args[1];
        postUrl = postUrl.replaceAll("\\{.*\\}", "");
        System.out.println("Post url: " + postUrl);

        Map<String, File> files = new HashMap<>();

        for (var f : new File("build/libs").listFiles()) {
            files.put(f.getName(), f);
        }

        for (var f : new File("tmp/exec").listFiles()) {
            files.put(f.getName(), f);
        }


        byte[] buf = new byte[20480];


        for (Map.Entry<String, File> fileEntry : files.entrySet()) {
            URL u = new URL(postUrl + "?name=" + URLEncoder.encode(fileEntry.getKey(), StandardCharsets.UTF_8));
            HttpURLConnection connection = (HttpURLConnection) u.openConnection();
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Accept", "application/vnd.github.v3+json");
            connection.setRequestProperty("Content-Length", String.valueOf(fileEntry.getValue().length()));
            connection.setRequestProperty("Content-Type", "application/zip");
            connection.setRequestProperty("Authorization", "token " + System.getenv("GH_TOKEN"));

            connection.setDoOutput(true);
            {
                OutputStream outputStream = connection.getOutputStream();
                try (InputStream is = new BufferedInputStream(new FileInputStream(fileEntry.getValue()))) {
                    while (true) {
                        int len = is.read(buf);
                        if (len == -1) break;
                        outputStream.write(buf, 0, len);
                        outputStream.flush();
                    }
                }
                outputStream.close();
            }

            if (connection.getResponseCode() != 201) {
                int len = 20480;
                try {
                    len = Math.max(len, connection.getHeaderFieldInt("Content-Length", len));
                } catch (Throwable ignore) {
                }
                ByteArrayOutputStream bos = new ByteArrayOutputStream(len);
                InputStream is;
                try {
                    is = connection.getErrorStream();
                } catch (Throwable ignore) {
                    is = connection.getInputStream();
                }
                while (true) {
                    int lenx = is.read(buf);
                    if (lenx == -1) break;
                    bos.write(buf, 0, lenx);
                }
                throw new IOException(bos.toString(StandardCharsets.UTF_8));
            }
        }
    }
}
