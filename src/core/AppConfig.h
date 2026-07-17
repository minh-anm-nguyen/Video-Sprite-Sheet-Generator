#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace vss {

struct AppConfig {
    std::filesystem::path inputPath;
    std::optional<double> intervalSec;
    int cols = 5;
    int rows = 5;
    int cellWidth = 160;
    std::filesystem::path imagePath;
    std::filesystem::path vttPath;
    int jpegQuality = 85;
    std::string baseUrl;
    bool force = false;
    bool quiet = false;

    double effectiveInterval(double durationSec) const;
    std::filesystem::path sheetPath(int index, int sheetCount) const;
};

}
