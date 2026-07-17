#include <initializer_list>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "cli/CliParser.h"
#include "core/AppError.h"

using namespace vss;

namespace {

std::optional<AppConfig> parseRaw(std::initializer_list<const char*> args) {
    std::vector<const char*> argv{"video-sprite-sheet"};
    argv.insert(argv.end(), args.begin(), args.end());
    return CliParser::parse(static_cast<int>(argv.size()), argv.data());
}

AppConfig parseOk(std::initializer_list<const char*> args) {
    std::optional<AppConfig> config = parseRaw(args);
    REQUIRE(config.has_value());
    return *config;
}

bool failsAsArgument(std::initializer_list<const char*> args) {
    try {
        parseRaw(args);
    } catch (const AppError& e) {
        return e.category() == ErrorCategory::Argument;
    }
    return false;
}

}

TEST_CASE("a minimal command applies every default") {
    AppConfig config = parseOk({"video.mp4"});
    CHECK(config.inputPath == std::filesystem::path("video.mp4"));
    CHECK_FALSE(config.intervalSec.has_value());
    CHECK(config.cols == 5);
    CHECK(config.rows == 5);
    CHECK(config.cellWidth == 160);
    CHECK(config.jpegQuality == 85);
    CHECK(config.imagePath == std::filesystem::path("video_sprite.jpg"));
    CHECK(config.vttPath == std::filesystem::path("video_sprite.vtt"));
    CHECK(config.baseUrl.empty());
    CHECK_FALSE(config.force);
    CHECK_FALSE(config.quiet);
}

TEST_CASE("every option is parsed with its short or long name") {
    AppConfig config = parseOk({"video.mp4", "-i", "2.5", "-c", "4", "-r", "6", "-w", "200",
                                "-o", "out/thumbs.png", "--vtt", "out/thumbs.vtt", "-q", "90",
                                "--base-url", "https://cdn.example.com/previews", "--force",
                                "--quiet"});
    CHECK(config.intervalSec == 2.5);
    CHECK(config.cols == 4);
    CHECK(config.rows == 6);
    CHECK(config.cellWidth == 200);
    CHECK(config.imagePath == std::filesystem::path("out/thumbs.png"));
    CHECK(config.vttPath == std::filesystem::path("out/thumbs.vtt"));
    CHECK(config.jpegQuality == 90);
    CHECK(config.baseUrl == "https://cdn.example.com/previews");
    CHECK(config.force);
    CHECK(config.quiet);

    AppConfig longNames = parseOk({"video.mp4", "--interval", "10", "--cols", "3", "--rows",
                                   "2", "--thumb-width", "320", "--output", "x.jpeg",
                                   "--quality", "50"});
    CHECK(longNames.intervalSec == 10.0);
    CHECK(longNames.cols == 3);
    CHECK(longNames.rows == 2);
    CHECK(longNames.cellWidth == 320);
    CHECK(longNames.imagePath == std::filesystem::path("x.jpeg"));
    CHECK(longNames.jpegQuality == 50);
}

TEST_CASE("overwrite is an alias for force") {
    CHECK(parseOk({"video.mp4", "--overwrite"}).force);
}

TEST_CASE("derived output paths follow the input name") {
    AppConfig config = parseOk({"clips/holiday.mkv"});
    CHECK(config.imagePath == std::filesystem::path("holiday_sprite.jpg"));
    CHECK(config.vttPath == std::filesystem::path("holiday_sprite.vtt"));
}

TEST_CASE("uppercase image extensions are accepted") {
    CHECK(parseOk({"video.mp4", "-o", "OUT.JPG"}).imagePath ==
          std::filesystem::path("OUT.JPG"));
    CHECK(parseOk({"video.mp4", "-o", "out.JPEG"}).imagePath ==
          std::filesystem::path("out.JPEG"));
}

TEST_CASE("help wins over every other argument and returns no config") {
    CHECK_FALSE(parseRaw({"--help"}).has_value());
    CHECK_FALSE(parseRaw({"-h"}).has_value());
    CHECK_FALSE(parseRaw({"video.mp4", "-q", "9999", "--help"}).has_value());
}

TEST_CASE("missing or duplicated positional input is rejected") {
    CHECK(failsAsArgument({}));
    CHECK(failsAsArgument({"a.mp4", "b.mp4"}));
}

TEST_CASE("unknown options are rejected") {
    CHECK(failsAsArgument({"video.mp4", "--unknown"}));
    CHECK(failsAsArgument({"video.mp4", "-z"}));
}

TEST_CASE("options that need a value reject a missing or malformed one") {
    CHECK(failsAsArgument({"video.mp4", "-i"}));
    CHECK(failsAsArgument({"video.mp4", "-c", "abc"}));
    CHECK(failsAsArgument({"video.mp4", "-i", "1.5x"}));
    CHECK(failsAsArgument({"video.mp4", "-q", "ninety"}));
}

TEST_CASE("out of range values are rejected") {
    CHECK(failsAsArgument({"video.mp4", "-q", "0"}));
    CHECK(failsAsArgument({"video.mp4", "-q", "101"}));
    CHECK(failsAsArgument({"video.mp4", "-c", "0"}));
    CHECK(failsAsArgument({"video.mp4", "-r", "-1"}));
    CHECK(failsAsArgument({"video.mp4", "-i", "0"}));
    CHECK(failsAsArgument({"video.mp4", "-i", "-2"}));
}

TEST_CASE("thumb width must be even and at least sixteen") {
    CHECK(failsAsArgument({"video.mp4", "-w", "14"}));
    CHECK(failsAsArgument({"video.mp4", "-w", "161"}));
    CHECK(parseOk({"video.mp4", "-w", "16"}).cellWidth == 16);
}

TEST_CASE("unsupported image extensions are rejected") {
    CHECK(failsAsArgument({"video.mp4", "-o", "preview.webp"}));
    CHECK(failsAsArgument({"video.mp4", "-o", "preview.bmp"}));
    CHECK(failsAsArgument({"video.mp4", "-o", "preview"}));
}

TEST_CASE("identical image and vtt paths are rejected") {
    CHECK(failsAsArgument({"video.mp4", "-o", "out.jpg", "--vtt", "out.jpg"}));
}
