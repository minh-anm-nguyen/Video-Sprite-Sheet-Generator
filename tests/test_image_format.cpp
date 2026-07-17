#include <catch2/catch_test_macros.hpp>

#include "media/ImageFormat.h"

using namespace vss;

TEST_CASE("jpg and jpeg extensions map to JPEG") {
    CHECK(formatFromExtension("a.jpg") == ImageFormat::Jpeg);
    CHECK(formatFromExtension("a.jpeg") == ImageFormat::Jpeg);
    CHECK(formatFromExtension("out/dir/a.JPG") == ImageFormat::Jpeg);
    CHECK(formatFromExtension("a.JpEg") == ImageFormat::Jpeg);
}

TEST_CASE("png extension maps to PNG") {
    CHECK(formatFromExtension("a.png") == ImageFormat::Png);
    CHECK(formatFromExtension("a.PNG") == ImageFormat::Png);
}

TEST_CASE("unsupported extensions are not recognized") {
    CHECK_FALSE(formatFromExtension("a.webp").has_value());
    CHECK_FALSE(formatFromExtension("a.bmp").has_value());
    CHECK_FALSE(formatFromExtension("a").has_value());
    CHECK_FALSE(formatFromExtension("a.").has_value());
    CHECK_FALSE(formatFromExtension("").has_value());
}
