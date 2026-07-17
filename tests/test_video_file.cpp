#include <filesystem>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/AppError.h"
#include "core/VideoInfo.h"
#include "media/Frame.h"
#include "media/VideoFile.h"

using namespace vss;

namespace {

std::filesystem::path asset(const char* name) {
    return std::filesystem::path(VSS_TEST_ASSETS_DIR) / name;
}

bool haveAssets() {
    return std::filesystem::exists(asset("sample_10s.mp4"));
}

std::string inputErrorMessage(const char* name) {
    VideoFile video;
    try {
        video.open(asset(name));
    } catch (const AppError& e) {
        REQUIRE(e.category() == ErrorCategory::Input);
        return e.what();
    }
    return {};
}

}

TEST_CASE("open reads metadata from a landscape video") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("sample_10s.mp4"));
    CHECK(info.width == 640);
    CHECK(info.height == 360);
    CHECK(info.duration > 9.5);
    CHECK(info.duration < 10.5);
    CHECK(info.aspectRatio > 1.7);
    CHECK(info.aspectRatio < 1.85);
    CHECK(info.rotationDegrees == 0);
}

TEST_CASE("open reads rotation metadata and swaps the display aspect") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("rotated_90.mp4"));
    CHECK(info.rotationDegrees % 180 == 90);
    CHECK(info.aspectRatio < 1.0);
}

TEST_CASE("open handles a portrait video") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("portrait_9x16.mp4"));
    CHECK(info.width == 360);
    CHECK(info.height == 640);
    CHECK(info.aspectRatio < 1.0);
    CHECK(info.rotationDegrees == 0);
}

TEST_CASE("open reports distinct input errors") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    std::string missing = inputErrorMessage("no_such_file.mp4");
    std::string noVideo = inputErrorMessage("audio_only.mp4");
    std::string garbage = inputErrorMessage("garbage.mp4");

    CHECK(missing.find("does not exist") != std::string::npos);
    CHECK(noVideo.find("no video stream") != std::string::npos);
    CHECK_FALSE(garbage.empty());
    CHECK(garbage != missing);
    CHECK(garbage != noVideo);
}

TEST_CASE("decodeFrameAtOrAfter returns the first frame at or after the target") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    video.open(asset("sample_10s.mp4"));
    REQUIRE(video.seekTo(5.0));
    std::optional<Frame> frame = video.decodeFrameAtOrAfter(5.0);
    REQUIRE(frame.has_value());
    CHECK(frame->timestampSec() >= 5.0);
    CHECK(frame->timestampSec() < 5.5);
    CHECK(frame->width() == 640);
    CHECK(frame->height() == 360);
}

TEST_CASE("frames near the end of the file are recovered by draining") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("sample_10s.mp4"));
    double nearEnd = info.duration - 0.2;
    REQUIRE(video.seekTo(nearEnd));
    std::optional<Frame> frame = video.decodeFrameAtOrAfter(nearEnd);
    REQUIRE(frame.has_value());
    CHECK(frame->timestampSec() >= nearEnd);
}

TEST_CASE("a target beyond the last frame yields no frame") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("sample_10s.mp4"));
    video.seekTo(info.duration);
    CHECK_FALSE(video.decodeFrameAtOrAfter(info.duration + 1.0).has_value());
}

TEST_CASE("seeking backwards flushes stale decoder state") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("sample_10s.mp4"));
    video.seekTo(info.duration - 0.2);
    video.decodeFrameAtOrAfter(info.duration - 0.2);

    REQUIRE(video.seekTo(1.0));
    std::optional<Frame> frame = video.decodeFrameAtOrAfter(1.0);
    REQUIRE(frame.has_value());
    CHECK(frame->timestampSec() >= 1.0);
    CHECK(frame->timestampSec() < 1.5);
}
