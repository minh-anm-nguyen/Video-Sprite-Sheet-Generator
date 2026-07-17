#include <catch2/catch_test_macros.hpp>

#include "core/AppConfig.h"

using namespace vss;

TEST_CASE("effectiveInterval returns the user value when present") {
    AppConfig config;
    config.intervalSec = 2.5;
    CHECK(config.effectiveInterval(100.0) == 2.5);
}

TEST_CASE("effectiveInterval divides the duration across the grid when absent") {
    AppConfig config;
    CHECK(config.effectiveInterval(100.0) == 4.0);
    config.cols = 4;
    config.rows = 5;
    CHECK(config.effectiveInterval(100.0) == 5.0);
}

TEST_CASE("sheetPath keeps the original name for a single sheet") {
    AppConfig config;
    config.imagePath = "video_sprite.jpg";
    CHECK(config.sheetPath(0, 1) == std::filesystem::path("video_sprite.jpg"));
}

TEST_CASE("sheetPath appends a zero padded one based index for multiple sheets") {
    AppConfig config;
    config.imagePath = "video_sprite.jpg";
    CHECK(config.sheetPath(0, 3) == std::filesystem::path("video_sprite_001.jpg"));
    CHECK(config.sheetPath(1, 3) == std::filesystem::path("video_sprite_002.jpg"));
    CHECK(config.sheetPath(2, 3) == std::filesystem::path("video_sprite_003.jpg"));
}

TEST_CASE("sheetPath widens the padding when sheets exceed three digits") {
    AppConfig config;
    config.imagePath = "video_sprite.jpg";
    CHECK(config.sheetPath(0, 10000) == std::filesystem::path("video_sprite_00001.jpg"));
    CHECK(config.sheetPath(9999, 10000) == std::filesystem::path("video_sprite_10000.jpg"));
}

TEST_CASE("sheetPath preserves the directory and extension") {
    AppConfig config;
    config.imagePath = std::filesystem::path("out") / "thumbs.png";
    CHECK(config.sheetPath(1, 2) == std::filesystem::path("out") / "thumbs_002.png");
}
