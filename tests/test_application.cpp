#include <cstddef>
#include <filesystem>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "app/Application.h"
#include "core/AppConfig.h"
#include "core/AppError.h"
#include "fakes/FakeVideoSource.h"
#include "fakes/MemoryFileWriter.h"

using namespace vss;

namespace {

AppConfig makeConfig() {
    AppConfig config;
    config.inputPath = "video.mp4";
    config.imagePath = "video_sprite.jpg";
    config.vttPath = "video_sprite.vtt";
    return config;
}

std::size_t countCues(const std::string& vtt) {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = vtt.find("-->", pos)) != std::string::npos) {
        ++count;
        pos += 3;
    }
    return count;
}

bool isJpeg(const std::vector<std::uint8_t>& bytes) {
    return bytes.size() > 2 && bytes[0] == 0xFF && bytes[1] == 0xD8;
}

}

TEST_CASE("a full run writes one sprite sheet and one vtt file") {
    FakeVideoSource source;
    MemoryFileWriter writer;
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::Success);
    REQUIRE(writer.files().size() == 2);
    REQUIRE(writer.files().contains("video_sprite.jpg"));
    REQUIRE(writer.files().contains("video_sprite.vtt"));
    CHECK(isJpeg(writer.files().at("video_sprite.jpg")));

    std::string vtt = writer.textOf("video_sprite.vtt");
    CHECK(vtt.starts_with("WEBVTT\n"));
    CHECK(countCues(vtt) == 25);
    CHECK(vtt.find("video_sprite.jpg#xywh=0,0,160,90") != std::string::npos);
}

TEST_CASE("more marks than one grid produces numbered sheets and one vtt") {
    AppConfig config = makeConfig();
    config.intervalSec = 2.0;
    FakeVideoSource source;
    source.info.duration = 124.0;
    MemoryFileWriter writer;
    Application app(config, source, writer);

    CHECK(app.run() == ExitCode::Success);
    REQUIRE(writer.files().size() == 4);
    CHECK(writer.files().contains("video_sprite_001.jpg"));
    CHECK(writer.files().contains("video_sprite_002.jpg"));
    CHECK(writer.files().contains("video_sprite_003.jpg"));
    CHECK(writer.files().contains("video_sprite.vtt"));

    std::string vtt = writer.textOf("video_sprite.vtt");
    CHECK(countCues(vtt) == 62);
    CHECK(vtt.find("video_sprite_003.jpg#xywh=") != std::string::npos);
}

TEST_CASE("decode failures midway still produce both outputs") {
    FakeVideoSource source;
    source.failFrom = 50.0;
    MemoryFileWriter writer;
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::Success);
    CHECK(writer.files().size() == 2);
    CHECK(countCues(writer.textOf("video_sprite.vtt")) == 25);
}

TEST_CASE("a video with no decodable frame fails and leaves no output") {
    FakeVideoSource source;
    source.failFrom = 0.0;
    MemoryFileWriter writer;
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::InputError);
    CHECK(writer.files().empty());
}

TEST_CASE("a video shorter than the interval yields a single cell sprite") {
    AppConfig config = makeConfig();
    config.intervalSec = 10.0;
    FakeVideoSource source;
    source.info.duration = 3.0;
    MemoryFileWriter writer;
    Application app(config, source, writer);

    CHECK(app.run() == ExitCode::Success);
    std::string vtt = writer.textOf("video_sprite.vtt");
    CHECK(countCues(vtt) == 1);
    CHECK(vtt.find("00:00:00.000 --> 00:00:03.000") != std::string::npos);
    CHECK(vtt.find("#xywh=0,0,160,90") != std::string::npos);
}

TEST_CASE("an existing output stops the run before any decoding") {
    FakeVideoSource source;
    MemoryFileWriter writer;
    writer.preload("video_sprite.jpg");
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::InputError);
    CHECK(source.decodeCalls == 0);
    CHECK(writer.created().empty());
}

TEST_CASE("an unopenable input fails with the input exit code") {
    FakeVideoSource source;
    source.failOpen = true;
    MemoryFileWriter writer;
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::InputError);
    CHECK(writer.files().empty());
}

TEST_CASE("seek failures turn into black cells instead of aborting") {
    FakeVideoSource source;
    source.failSeek = true;
    MemoryFileWriter writer;
    Application app(makeConfig(), source, writer);

    CHECK(app.run() == ExitCode::InputError);
    CHECK(writer.files().empty());
    CHECK(source.decodeCalls == 0);
}
