#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include "core/GridLayout.h"
#include "core/Rect.h"
#include "media/Frame.h"
#include "media/FrameScaler.h"
#include "media/ImageEncoder.h"
#include "media/ImageFormat.h"
#include "media/SpriteSheet.h"
#include "media/VideoFile.h"

using namespace vss;

namespace {

using Rgb = std::array<std::uint8_t, 3>;

constexpr Rgb kRed{255, 0, 0};
constexpr Rgb kGreen{0, 255, 0};
constexpr Rgb kBlue{0, 0, 255};

Frame makeRgbFrame(int width, int height, const std::function<Rgb(int, int)>& colorAt) {
    AVFrame* frame = av_frame_alloc();
    REQUIRE(frame != nullptr);
    frame->format = AV_PIX_FMT_RGB24;
    frame->width = width;
    frame->height = height;
    REQUIRE(av_frame_get_buffer(frame, 0) == 0);
    for (int y = 0; y < height; ++y) {
        std::uint8_t* row = frame->data[0] + static_cast<std::size_t>(y) * frame->linesize[0];
        for (int x = 0; x < width; ++x) {
            Rgb color = colorAt(x, y);
            row[x * 3] = color[0];
            row[x * 3 + 1] = color[1];
            row[x * 3 + 2] = color[2];
        }
    }
    return Frame(frame, 1.5);
}

Frame makeSolidFrame(int width, int height, Rgb color) {
    return makeRgbFrame(width, height, [color](int, int) { return color; });
}

Rgb frameRgbAt(const Frame& frame, int x, int y) {
    const AVFrame* f = frame.handle();
    const std::uint8_t* p = f->data[0] + static_cast<std::size_t>(y) * f->linesize[0] + x * 3;
    return Rgb{p[0], p[1], p[2]};
}

Rgb sheetRgbAt(const SpriteSheet& sheet, int x, int y) {
    const std::uint8_t* p = sheet.data() + (static_cast<std::size_t>(y) * sheet.width() + x) * 3;
    return Rgb{p[0], p[1], p[2]};
}

bool isNear(Rgb actual, Rgb expected, int tolerance = 24) {
    return std::abs(actual[0] - expected[0]) <= tolerance &&
           std::abs(actual[1] - expected[1]) <= tolerance &&
           std::abs(actual[2] - expected[2]) <= tolerance;
}

std::filesystem::path asset(const char* name) {
    return std::filesystem::path(VSS_TEST_ASSETS_DIR) / name;
}

bool haveAssets() {
    return std::filesystem::exists(asset("sample_10s.mp4"));
}

}

TEST_CASE("a frame matching the cell aspect fills the whole cell") {
    FrameScaler scaler(160, 90);
    Frame result = scaler.normalize(makeSolidFrame(640, 360, kRed), 0);
    CHECK(result.width() == 160);
    CHECK(result.height() == 90);
    CHECK(result.timestampSec() == 1.5);
    CHECK(isNear(frameRgbAt(result, 0, 0), kRed));
    CHECK(isNear(frameRgbAt(result, 80, 45), kRed));
    CHECK(isNear(frameRgbAt(result, 159, 89), kRed));
}

TEST_CASE("a 4x3 frame gets centered pillarbox bars in a 16x9 cell") {
    FrameScaler scaler(160, 90);
    Frame result = scaler.normalize(makeSolidFrame(640, 480, kRed), 0);
    CHECK(result.width() == 160);
    CHECK(result.height() == 90);
    CHECK(isNear(frameRgbAt(result, 5, 45), Rgb{0, 0, 0}, 4));
    CHECK(isNear(frameRgbAt(result, 154, 45), Rgb{0, 0, 0}, 4));
    CHECK(isNear(frameRgbAt(result, 80, 45), kRed));
}

TEST_CASE("a portrait frame gets wide pillarbox bars") {
    FrameScaler scaler(160, 90);
    Frame result = scaler.normalize(makeSolidFrame(360, 640, kGreen), 0);
    CHECK(result.width() == 160);
    CHECK(result.height() == 90);
    CHECK(isNear(frameRgbAt(result, 20, 45), Rgb{0, 0, 0}, 4));
    CHECK(isNear(frameRgbAt(result, 140, 45), Rgb{0, 0, 0}, 4));
    CHECK(isNear(frameRgbAt(result, 80, 45), kGreen));
}

TEST_CASE("rotation is applied before fitting") {
    FrameScaler scaler(160, 90);
    Frame source = makeRgbFrame(200, 100, [](int x, int) {
        return x < 100 ? kRed : kBlue;
    });
    Frame result = scaler.normalize(source, 90);
    CHECK(result.width() == 160);
    CHECK(result.height() == 90);
    CHECK(isNear(frameRgbAt(result, 80, 20), kRed));
    CHECK(isNear(frameRgbAt(result, 80, 70), kBlue));
    CHECK(isNear(frameRgbAt(result, 20, 45), Rgb{0, 0, 0}, 4));
}

TEST_CASE("a 180 degree rotation keeps dimensions and flips content") {
    FrameScaler scaler(160, 90);
    Frame source = makeRgbFrame(640, 360, [](int x, int) {
        return x < 320 ? kRed : kBlue;
    });
    Frame result = scaler.normalize(source, 180);
    CHECK(isNear(frameRgbAt(result, 40, 45), kBlue));
    CHECK(isNear(frameRgbAt(result, 120, 45), kRed));
}

TEST_CASE("the sprite canvas starts black") {
    GridLayout layout(2, 2, 160, 16.0 / 9.0);
    SpriteSheet sheet(layout, 0);
    CHECK(sheet.width() == 320);
    CHECK(sheet.height() == 180);
    CHECK(sheetRgbAt(sheet, 0, 0) == Rgb{0, 0, 0});
    CHECK(sheetRgbAt(sheet, 319, 179) == Rgb{0, 0, 0});
    CHECK(sheetRgbAt(sheet, 200, 120) == Rgb{0, 0, 0});
}

TEST_CASE("place copies a thumbnail into its cell and nowhere else") {
    GridLayout layout(2, 2, 160, 16.0 / 9.0);
    SpriteSheet sheet(layout, 0);
    Rect cell = layout.cellRect(3);
    sheet.place(makeSolidFrame(cell.w, cell.h, kGreen), cell);

    CHECK(sheetRgbAt(sheet, cell.x, cell.y) == kGreen);
    CHECK(sheetRgbAt(sheet, cell.x + cell.w - 1, cell.y + cell.h - 1) == kGreen);
    CHECK(sheetRgbAt(sheet, cell.x - 1, cell.y) == Rgb{0, 0, 0});
    CHECK(sheetRgbAt(sheet, 10, 10) == Rgb{0, 0, 0});
}

TEST_CASE("place respects the source frame stride") {
    GridLayout layout(2, 1, 150, 150.0 / 84.0);
    SpriteSheet sheet(layout, 0);
    Rect cell = layout.cellRect(0);
    REQUIRE(cell.w == 150);
    sheet.place(makeSolidFrame(cell.w, cell.h, kBlue), cell);

    CHECK(sheetRgbAt(sheet, 0, 0) == kBlue);
    CHECK(sheetRgbAt(sheet, 149, cell.h - 1) == kBlue);
    CHECK(sheetRgbAt(sheet, 150, 0) == Rgb{0, 0, 0});
}

TEST_CASE("fillBlack clears a cell") {
    GridLayout layout(2, 2, 160, 16.0 / 9.0);
    SpriteSheet sheet(layout, 0);
    Rect cell = layout.cellRect(0);
    sheet.place(makeSolidFrame(cell.w, cell.h, kRed), cell);
    sheet.fillBlack(cell);
    CHECK(sheetRgbAt(sheet, 10, 10) == Rgb{0, 0, 0});
    CHECK(sheetRgbAt(sheet, cell.w - 1, cell.h - 1) == Rgb{0, 0, 0});
}

TEST_CASE("jpeg encoding produces a jpeg with quality dependent size") {
    GridLayout layout(2, 2, 160, 16.0 / 9.0);
    SpriteSheet sheet(layout, 0);
    for (int i = 0; i < 4; ++i) {
        Rect cell = layout.cellRect(i);
        Rgb color = i % 2 == 0 ? kRed : kGreen;
        sheet.place(makeRgbFrame(cell.w, cell.h,
                                 [color](int x, int y) {
                                     return (x / 8 + y / 8) % 2 == 0 ? color : kBlue;
                                 }),
                    cell);
    }

    std::vector<std::uint8_t> high = ImageEncoder(ImageFormat::Jpeg, 90).encode(sheet);
    std::vector<std::uint8_t> low = ImageEncoder(ImageFormat::Jpeg, 10).encode(sheet);

    REQUIRE(high.size() > 2);
    CHECK(high[0] == 0xFF);
    CHECK(high[1] == 0xD8);
    CHECK(high.size() > low.size());
    CHECK_NOTHROW(ImageEncoder(ImageFormat::Jpeg, 1).encode(sheet));
    CHECK_NOTHROW(ImageEncoder(ImageFormat::Jpeg, 100).encode(sheet));
}

TEST_CASE("png encoding produces a png") {
    GridLayout layout(1, 1, 160, 16.0 / 9.0);
    SpriteSheet sheet(layout, 0);
    Rect cell = layout.cellRect(0);
    sheet.place(makeSolidFrame(cell.w, cell.h, kBlue), cell);

    std::vector<std::uint8_t> bytes = ImageEncoder(ImageFormat::Png, 85).encode(sheet);
    REQUIRE(bytes.size() > 8);
    CHECK(bytes[0] == 0x89);
    CHECK(bytes[1] == 0x50);
    CHECK(bytes[2] == 0x4E);
    CHECK(bytes[3] == 0x47);
}

TEST_CASE("a real video renders a 2x2 sprite for visual inspection") {
    if (!haveAssets()) {
        SKIP("test assets missing, run scripts/make_test_videos.ps1 first");
    }
    VideoFile video;
    VideoInfo info = video.open(asset("sample_10s.mp4"));
    GridLayout layout(2, 2, 160, info.aspectRatio);
    FrameScaler scaler(layout.cellWidth(), layout.cellHeight());
    SpriteSheet sheet(layout, 0);

    for (int i = 0; i < 4; ++i) {
        double t = i * 2.5;
        REQUIRE(video.seekTo(t));
        std::optional<Frame> frame = video.decodeFrameAtOrAfter(t);
        REQUIRE(frame.has_value());
        sheet.place(scaler.normalize(*frame, info.rotationDegrees), layout.cellRect(i));
    }

    std::filesystem::path outDir = std::filesystem::path(VSS_TEST_ASSETS_DIR) / "out";
    std::filesystem::create_directories(outDir);
    std::vector<std::uint8_t> jpeg = ImageEncoder(ImageFormat::Jpeg, 85).encode(sheet);
    std::vector<std::uint8_t> png = ImageEncoder(ImageFormat::Png, 85).encode(sheet);
    std::ofstream(outDir / "visual_grid.jpg", std::ios::binary)
        .write(reinterpret_cast<const char*>(jpeg.data()),
               static_cast<std::streamsize>(jpeg.size()));
    std::ofstream(outDir / "visual_grid.png", std::ios::binary)
        .write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));

    CHECK(std::filesystem::file_size(outDir / "visual_grid.jpg") > 1000);
    CHECK(std::filesystem::file_size(outDir / "visual_grid.png") > 1000);
}
