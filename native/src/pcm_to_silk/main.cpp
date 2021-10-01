#include "SilkCoder.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

int main(int argc, const char **args) {
    if (argc != 4) {
        std::cerr << "Usage:" << std::endl
                  << "" << std::endl
                  << "pcm_to_silk [input] [sampleRate] [output]" << std::endl
                  << std::endl;
        return 1;
    }

    auto simpleRate = std::atol(args[2]);
    FILE *src = fopen(args[1], "rb");
    FILE *target = fopen(args[3], "wb");

    std::cerr << "Processing....." << std::endl;

    SilkerCoder_encode(
            NATIVE_FILE_SYSTEM(),
            NULL,
            (void **) src,
            (void **) target,
            true, true, false,
            simpleRate,
            24000,
            20 * simpleRate / 1000,
            0, 0, 0, 2,
            24000
    );
}

#pragma clang diagnostic pop