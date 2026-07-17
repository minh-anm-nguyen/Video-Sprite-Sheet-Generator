#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/AppError.h"
#include "output/AtomicFileWriter.h"

using namespace vss;

namespace {

struct TempDir {
    std::filesystem::path path;

    TempDir() {
        path = std::filesystem::temp_directory_path() /
               ("vss_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::vector<std::uint8_t> asBytes(std::string_view text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::string readAll(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::size_t entryCount(const std::filesystem::path& dir) {
    std::size_t count = 0;
    for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator(dir)) {
        ++count;
    }
    return count;
}

bool failsAsInput(AtomicFileWriter& writer,
                  const std::vector<std::filesystem::path>& paths) {
    try {
        writer.precheck(paths);
    } catch (const AppError& e) {
        return e.category() == ErrorCategory::Input;
    }
    return false;
}

}

TEST_CASE("write stores the exact bytes and leaves no temp file behind") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path target = dir.path / "sheet.jpg";

    writer.write(target, asBytes("sprite-bytes"));

    CHECK(readAll(target) == "sprite-bytes");
    CHECK(entryCount(dir.path) == 1);
    CHECK(writer.created() == std::vector<std::filesystem::path>{target});
}

TEST_CASE("write creates missing parent directories") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path target = dir.path / "a" / "b" / "sheet.png";

    writer.write(target, asBytes("x"));

    CHECK(std::filesystem::exists(target));
    CHECK(readAll(target) == "x");
}

TEST_CASE("write accepts empty content") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path target = dir.path / "empty.vtt";

    writer.write(target, {});

    CHECK(std::filesystem::exists(target));
    CHECK(std::filesystem::file_size(target) == 0);
}

TEST_CASE("precheck rejects an existing output unless force is set") {
    TempDir dir;
    std::filesystem::path existing = dir.path / "sheet.jpg";
    std::ofstream(existing) << "old";
    std::filesystem::path fresh = dir.path / "new.jpg";

    AtomicFileWriter writer(false);
    CHECK(failsAsInput(writer, {fresh, existing}));
    CHECK_NOTHROW(writer.precheck({fresh}));
    CHECK(readAll(existing) == "old");

    AtomicFileWriter forced(true);
    CHECK_NOTHROW(forced.precheck({fresh, existing}));
}

TEST_CASE("write replaces an existing file in force mode") {
    TempDir dir;
    std::filesystem::path target = dir.path / "sheet.jpg";
    std::ofstream(target) << "old-content";

    AtomicFileWriter writer(true);
    writer.precheck({target});
    writer.write(target, asBytes("new-content"));

    CHECK(readAll(target) == "new-content");
    CHECK(entryCount(dir.path) == 1);
}

TEST_CASE("removeAllCreated deletes every file written in the run") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path first = dir.path / "sheet_001.jpg";
    std::filesystem::path second = dir.path / "sheet_002.jpg";
    writer.write(first, asBytes("a"));
    writer.write(second, asBytes("b"));

    writer.removeAllCreated();

    CHECK_FALSE(std::filesystem::exists(first));
    CHECK_FALSE(std::filesystem::exists(second));
    CHECK(writer.created().empty());
}

TEST_CASE("removeAllCreated is best effort and never throws") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path target = dir.path / "sheet.jpg";
    writer.write(target, asBytes("a"));
    std::filesystem::remove(target);

    CHECK_NOTHROW(writer.removeAllCreated());
    CHECK(writer.created().empty());
}

TEST_CASE("a failed write does not leave a temp file behind") {
    TempDir dir;
    AtomicFileWriter writer(false);
    std::filesystem::path blocked = dir.path / "taken";
    std::filesystem::create_directories(blocked / "sub");

    CHECK_THROWS_AS(writer.write(blocked, asBytes("x")), AppError);
    CHECK(entryCount(dir.path) == 1);
    CHECK(writer.created().empty());
}
