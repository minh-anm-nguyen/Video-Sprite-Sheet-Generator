#include "cli/CliParser.h"

#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include <system_error>

#include "core/AppError.h"
#include "media/ImageFormat.h"

namespace vss {

namespace {

[[noreturn]] void fail(const std::string& message) {
    throw AppError(ErrorCategory::Argument, message);
}

bool isFlag(std::string_view arg, std::string_view longName, std::string_view shortName = {}) {
    return arg == longName || (!shortName.empty() && arg == shortName);
}

std::string_view nextValue(int argc, const char* const* argv, int& i, std::string_view flag) {
    if (i + 1 >= argc) {
        fail(std::string(flag) + " requires a value");
    }
    ++i;
    return argv[i];
}

int parseInt(std::string_view text, std::string_view flag) {
    int value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || ptr != text.data() + text.size()) {
        fail(std::string(flag) + " expects an integer, got '" + std::string(text) + "'");
    }
    return value;
}

double parseDouble(std::string_view text, std::string_view flag) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || ptr != text.data() + text.size()) {
        fail(std::string(flag) + " expects a number, got '" + std::string(text) + "'");
    }
    return value;
}

}

std::optional<AppConfig> CliParser::parse(int argc, const char* const* argv) {
    for (int i = 1; i < argc; ++i) {
        if (isFlag(argv[i], "--help", "-h")) {
            printHelp();
            return std::nullopt;
        }
    }

    AppConfig config;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (isFlag(arg, "--interval", "-i")) {
            config.intervalSec = parseDouble(nextValue(argc, argv, i, arg), arg);
        } else if (isFlag(arg, "--cols", "-c")) {
            config.cols = parseInt(nextValue(argc, argv, i, arg), arg);
        } else if (isFlag(arg, "--rows", "-r")) {
            config.rows = parseInt(nextValue(argc, argv, i, arg), arg);
        } else if (isFlag(arg, "--thumb-width", "-w")) {
            config.cellWidth = parseInt(nextValue(argc, argv, i, arg), arg);
        } else if (isFlag(arg, "--output", "-o")) {
            config.imagePath = std::string(nextValue(argc, argv, i, arg));
        } else if (isFlag(arg, "--vtt")) {
            config.vttPath = std::string(nextValue(argc, argv, i, arg));
        } else if (isFlag(arg, "--quality", "-q")) {
            config.jpegQuality = parseInt(nextValue(argc, argv, i, arg), arg);
        } else if (isFlag(arg, "--base-url")) {
            config.baseUrl = std::string(nextValue(argc, argv, i, arg));
        } else if (isFlag(arg, "--force") || isFlag(arg, "--overwrite")) {
            config.force = true;
        } else if (isFlag(arg, "--quiet")) {
            config.quiet = true;
        } else if (arg.size() > 1 && arg.front() == '-') {
            fail("unknown option '" + std::string(arg) + "', try --help");
        } else if (config.inputPath.empty()) {
            config.inputPath = std::string(arg);
        } else {
            fail("unexpected extra argument '" + std::string(arg) + "'");
        }
    }

    resolveOutputPaths(config);
    validate(config);
    return config;
}

void CliParser::resolveOutputPaths(AppConfig& config) {
    if (config.inputPath.empty()) {
        return;
    }
    std::string stem = config.inputPath.stem().string();
    if (config.imagePath.empty()) {
        config.imagePath = stem + "_sprite.jpg";
    }
    if (config.vttPath.empty()) {
        config.vttPath = stem + "_sprite.vtt";
    }
}

void CliParser::validate(const AppConfig& config) {
    if (config.inputPath.empty()) {
        fail("an input video file is required, try --help");
    }
    if (!formatFromExtension(config.imagePath).has_value()) {
        fail("unsupported image extension '" + config.imagePath.extension().string() +
             "', use .jpg, .jpeg or .png");
    }
    if (config.jpegQuality < 1 || config.jpegQuality > 100) {
        fail("--quality must be between 1 and 100, got " + std::to_string(config.jpegQuality));
    }
    if (config.cols < 1) {
        fail("--cols must be at least 1, got " + std::to_string(config.cols));
    }
    if (config.rows < 1) {
        fail("--rows must be at least 1, got " + std::to_string(config.rows));
    }
    if (config.cellWidth < 16 || config.cellWidth % 2 != 0) {
        fail("--thumb-width must be an even number of at least 16, got " +
             std::to_string(config.cellWidth));
    }
    if (config.intervalSec.has_value() && *config.intervalSec <= 0.0) {
        fail("--interval must be positive, got " + std::to_string(*config.intervalSec));
    }
    if (config.imagePath == config.vttPath) {
        fail("image and vtt output paths must differ");
    }
}

void CliParser::printHelp() {
    std::printf(
        "Usage: video-sprite-sheet <input> [options]\n"
        "\n"
        "Extracts frames from a video at even intervals, tiles them into sprite\n"
        "sheets and writes a WebVTT file for timeline hover previews.\n"
        "\n"
        "Options:\n"
        "  -i, --interval <seconds>  time between thumbnails (default: duration / (cols x rows))\n"
        "  -c, --cols <n>            grid columns (default: 5)\n"
        "  -r, --rows <n>            grid rows (default: 5)\n"
        "  -w, --thumb-width <px>    thumbnail width, even, at least 16 (default: 160)\n"
        "  -o, --output <file>       sprite image path, .jpg/.jpeg/.png (default: <input>_sprite.jpg)\n"
        "      --vtt <file>          WebVTT output path (default: <input>_sprite.vtt)\n"
        "  -q, --quality <1-100>     JPEG quality (default: 85)\n"
        "      --base-url <url>      prefix for image references inside the VTT file\n"
        "      --force, --overwrite  allow overwriting existing output files\n"
        "      --quiet               suppress progress output\n"
        "  -h, --help                show this help and exit\n"
        "\n"
        "Examples:\n"
        "  video-sprite-sheet video.mp4\n"
        "  video-sprite-sheet video.mp4 -i 10 -c 4 -r 6 -w 200 -o out/sprite.jpg\n"
        "  video-sprite-sheet video.mp4 --vtt out/sprite.vtt -q 90 --base-url https://cdn.example.com/previews\n");
}

}
