#include <clocale>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/AppConfig.h"
#include "core/GridLayout.h"
#include "core/VttCue.h"
#include "fakes/MemoryFileWriter.h"
#include "output/VttCueBuilder.h"
#include "output/WebVttWriter.h"

using namespace vss;

namespace {

constexpr double kAspect16x9 = 16.0 / 9.0;

AppConfig makeConfig() {
    AppConfig config;
    config.inputPath = "video.mp4";
    config.imagePath = "video_sprite.jpg";
    config.vttPath = "video_sprite.vtt";
    return config;
}

}

TEST_CASE("formatTimestamp renders hh:mm:ss.mmm with a dot separator") {
    CHECK(WebVttWriter::formatTimestamp(0.0) == "00:00:00.000");
    CHECK(WebVttWriter::formatTimestamp(10.0) == "00:00:10.000");
    CHECK(WebVttWriter::formatTimestamp(90.5) == "00:01:30.500");
    CHECK(WebVttWriter::formatTimestamp(3600.0) == "01:00:00.000");
    CHECK(WebVttWriter::formatTimestamp(100000.0) == "27:46:40.000");
}

TEST_CASE("formatTimestamp carries rounded milliseconds into the next second") {
    CHECK(WebVttWriter::formatTimestamp(59.9995) == "00:01:00.000");
    CHECK(WebVttWriter::formatTimestamp(0.0004) == "00:00:00.000");
}

TEST_CASE("formatTimestamp ignores the process locale") {
    std::string saved = std::setlocale(LC_NUMERIC, nullptr);
    bool switched = std::setlocale(LC_NUMERIC, "de-DE") != nullptr ||
                    std::setlocale(LC_NUMERIC, "vi-VN") != nullptr;
    std::string formatted = WebVttWriter::formatTimestamp(90.5);
    std::setlocale(LC_NUMERIC, saved.c_str());
    if (switched) {
        CHECK(formatted == "00:01:30.500");
    }
}

TEST_CASE("a full vtt file matches the specification example") {
    AppConfig config = makeConfig();
    GridLayout layout(5, 5, 160, kAspect16x9);
    VttCueBuilder builder(config, layout, 1);
    std::vector<VttCue> cues = builder.buildCues(4, 10.0, 35.0);

    CHECK(WebVttWriter::serialize(cues) ==
          "WEBVTT\n"
          "\n"
          "00:00:00.000 --> 00:00:10.000\n"
          "video_sprite.jpg#xywh=0,0,160,90\n"
          "\n"
          "00:00:10.000 --> 00:00:20.000\n"
          "video_sprite.jpg#xywh=160,0,160,90\n"
          "\n"
          "00:00:20.000 --> 00:00:30.000\n"
          "video_sprite.jpg#xywh=320,0,160,90\n"
          "\n"
          "00:00:30.000 --> 00:00:35.000\n"
          "video_sprite.jpg#xywh=480,0,160,90\n"
          "\n");
}

TEST_CASE("a single thumbnail yields one cue covering the whole video") {
    AppConfig config = makeConfig();
    GridLayout layout(5, 5, 160, kAspect16x9);
    VttCueBuilder builder(config, layout, 1);
    std::vector<VttCue> cues = builder.buildCues(1, 10.0, 5.0);

    REQUIRE(cues.size() == 1);
    CHECK(cues[0].start == 0.0);
    CHECK(cues[0].end == 5.0);
    CHECK(cues[0].imageRef == "video_sprite.jpg");
    CHECK(cues[0].region == Rect{0, 0, 160, 90});
}

TEST_CASE("cues spanning multiple sheets reference the right file and cell") {
    AppConfig config = makeConfig();
    GridLayout layout(5, 5, 160, kAspect16x9);
    VttCueBuilder builder(config, layout, 3);
    std::vector<VttCue> cues = builder.buildCues(62, 2.0, 124.0);

    REQUIRE(cues.size() == 62);
    CHECK(cues[0].imageRef == "video_sprite_001.jpg");
    CHECK(cues[0].region == Rect{0, 0, 160, 90});
    CHECK(cues[24].imageRef == "video_sprite_001.jpg");
    CHECK(cues[24].region == Rect{640, 360, 160, 90});
    CHECK(cues[25].imageRef == "video_sprite_002.jpg");
    CHECK(cues[25].region == Rect{0, 0, 160, 90});
    CHECK(cues[61].imageRef == "video_sprite_003.jpg");
    CHECK(cues[61].region == Rect{160, 180, 160, 90});
}

TEST_CASE("the base url is prefixed without doubling slashes") {
    AppConfig config = makeConfig();
    GridLayout layout(5, 5, 160, kAspect16x9);

    config.baseUrl = "https://cdn.example.com/previews";
    CHECK(VttCueBuilder(config, layout, 1).makeImageRef(0) ==
          "https://cdn.example.com/previews/video_sprite.jpg");

    config.baseUrl = "https://cdn.example.com/previews/";
    CHECK(VttCueBuilder(config, layout, 1).makeImageRef(0) ==
          "https://cdn.example.com/previews/video_sprite.jpg");
}

TEST_CASE("image references keep only the file name of the sheet path") {
    AppConfig config = makeConfig();
    config.imagePath = std::filesystem::path("out") / "thumbs.png";
    GridLayout layout(5, 5, 160, kAspect16x9);

    CHECK(VttCueBuilder(config, layout, 1).makeImageRef(0) == "thumbs.png");
    CHECK(VttCueBuilder(config, layout, 2).makeImageRef(1) == "thumbs_002.png");
}

TEST_CASE("write serializes through the injected file writer") {
    AppConfig config = makeConfig();
    GridLayout layout(5, 5, 160, kAspect16x9);
    VttCueBuilder builder(config, layout, 1);
    std::vector<VttCue> cues = builder.buildCues(1, 10.0, 5.0);

    MemoryFileWriter writer;
    WebVttWriter vtt(writer);
    vtt.write(cues, config.vttPath);

    REQUIRE(writer.files().contains(config.vttPath));
    CHECK(writer.textOf(config.vttPath) == WebVttWriter::serialize(cues));
    CHECK(writer.created() == std::vector<std::filesystem::path>{config.vttPath});
}
