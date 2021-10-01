package io.github.kasukusakura.silkcodec;

import java.io.IOException;

public class CoderException extends IOException {
    public CoderException() {
    }

    public CoderException(String message) {
        super(message);
    }

    public CoderException(String message, Throwable cause) {
        super(message, cause);
    }

    public CoderException(Throwable cause) {
        super(cause);
    }
}
