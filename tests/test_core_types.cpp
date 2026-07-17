#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/AppError.h"
#include "core/Rect.h"
#include "core/VideoInfo.h"
#include "core/VttCue.h"

using namespace vss;

TEST_CASE("AppError maps each category to its exit code") {
    CHECK(AppError(ErrorCategory::Argument, "a").exitCode() == ExitCode::ArgumentError);
    CHECK(AppError(ErrorCategory::Input, "i").exitCode() == ExitCode::InputError);
    CHECK(AppError(ErrorCategory::Processing, "p").exitCode() == ExitCode::ProcessingError);
}

TEST_CASE("AppError keeps category and message") {
    AppError err(ErrorCategory::Input, "file not found");
    CHECK(err.category() == ErrorCategory::Input);
    CHECK(std::string(err.what()) == "file not found");
}

TEST_CASE("ExitCode values match the CLI contract") {
    CHECK(static_cast<int>(ExitCode::Success) == 0);
    CHECK(static_cast<int>(ExitCode::ArgumentError) == 1);
    CHECK(static_cast<int>(ExitCode::InputError) == 2);
    CHECK(static_cast<int>(ExitCode::ProcessingError) == 3);
    CHECK(static_cast<int>(ExitCode::Interrupted) == 4);
}

TEST_CASE("Rect supports value equality") {
    CHECK(Rect{0, 0, 160, 90} == Rect{0, 0, 160, 90});
    CHECK_FALSE(Rect{0, 0, 160, 90} == Rect{160, 0, 160, 90});
}

TEST_CASE("VttCue holds timing, reference and region") {
    VttCue cue{10.0, 20.0, "video_sprite.jpg", Rect{160, 0, 160, 90}};
    CHECK(cue.start == 10.0);
    CHECK(cue.end == 20.0);
    CHECK(cue.imageRef == "video_sprite.jpg");
    CHECK(cue.region == Rect{160, 0, 160, 90});
}

TEST_CASE("VideoInfo defaults are zeroed") {
    VideoInfo info;
    CHECK(info.duration == 0.0);
    CHECK(info.width == 0);
    CHECK(info.height == 0);
    CHECK(info.aspectRatio == 0.0);
    CHECK(info.rotationDegrees == 0);
}
