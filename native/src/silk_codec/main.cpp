
#include "cxxopts.hpp"
#include <fstream>
#include "SilkCoder.h"
#include "math.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

int pts(int argc, const char **args) {
    cxxopts::Options options("pts", "");
    options.add_options()
            ("i,input", "PCM file", cxxopts::value<std::string>())//
            ("s,simple-rate", "simple rate of PCM", cxxopts::value<int>())//
            ("o,output", "output", cxxopts::value<std::string>())//
            ;


    auto opts = options.parse(argc, args);
    if (!opts.count("i") || !opts.count("s") || !opts.count("o")) {
        std::cerr << options.help() << std::endl;
        return 1;
    }

    auto inp = opts["input"].as<std::string>();
    auto rat = opts["simple-rate"].as<int>();
    auto output = opts["output"].as<std::string>();

    auto simpleRate = rat;
    auto src = fopen(inp.c_str(), "rb");
    auto dst = fopen(output.c_str(), "wb");

    std::cerr << "Processing..." << std::endl;
    std::cerr << "Silk simple rate: 24000" << std::endl;

    SilkerCoder_encode(
            NATIVE_FILE_SYSTEM(),
            NULL,
            (void **) src,
            (void **) dst,
            true, true, false,
            simpleRate,
            24000,
            20 * simpleRate / 1000,
            0, 0, 0, 2,
            24000
    );

    fclose(dst);
    fclose(src);

    return 0;
}

int stp(int argc, const char **args) {
    //SilkCoder_decode
    cxxopts::Options options("stp", "");
    options.add_options()//
            ("i,input", "silk file", cxxopts::value<std::string>())//
            ("o,output", "output", cxxopts::value<std::string>())//
            ("s,simple-rate", "simple rate of silk", cxxopts::value<int>()->default_value("24000"))//
            ("l,loss", "loss", cxxopts::value<int>()->default_value("0"))//
            ;

    auto opts = options.parse(argc, args);
    if (!opts.count("i") || !opts.count("s") || !opts.count("o")) {
        std::cerr << options.help() << std::endl;
        return 1;
    }

    auto inp = opts["input"].as<std::string>();
    auto rat = opts["simple-rate"].as<int>();
    auto loss = opts["loss"].as<int>();
    auto output = opts["output"].as<std::string>();

    auto simpleRate = rat;
    auto src = fopen(inp.c_str(), "rb");
    auto dst = fopen(output.c_str(), "wb");

    std::cerr << "Processing..." << std::endl;

    SilkCoder_decode(
            NATIVE_FILE_SYSTEM(),
            NULL,
            (void **) src,
            (void **) dst,
            true,
            true,
            simpleRate,
            loss
    );

    fclose(dst);
    fclose(src);

    return 0;
}

int main(int argc, const char **args) {
    if (argc < 1) {
        return 1;
    }

    if (argc > 1) {
        if (strcmp("pts", args[1]) == 0) {
            return pts(argc - 1, &args[1]);
        }
        if (strcmp("stp", args[1]) == 0) {
            return stp(argc - 1, &args[1]);
        }
    }

    std::cout
            << "Silk codec" << std::endl
            << std::endl
            << "Usage:" << std::endl
            << "silk-codec pts           Convert PCM to silk v3" << std::endl
            << "           stp           Convert silk v3 to PCM" << std::endl
            << std::endl;
    return 1;
}


#pragma clang diagnostic pop

