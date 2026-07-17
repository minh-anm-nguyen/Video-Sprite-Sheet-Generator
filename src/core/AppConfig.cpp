#include "core/AppConfig.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace vss {

double AppConfig::effectiveInterval(double durationSec) const {
    if (intervalSec.has_value()) {
        return *intervalSec;
    }
    return durationSec / (static_cast<double>(cols) * static_cast<double>(rows));
}

std::filesystem::path AppConfig::sheetPath(int index, int sheetCount) const {
    assert(index >= 0);
    assert(sheetCount >= 1);
    assert(index < sheetCount);
    if (sheetCount == 1) {
        return imagePath;
    }
    std::size_t width = std::max<std::size_t>(3, std::to_string(sheetCount).size());
    std::string label = std::to_string(index + 1);
    std::string padded = std::string(width - label.size(), '0') + label;
    std::filesystem::path result = imagePath;
    result.replace_filename(imagePath.stem().string() + "_" + padded +
                            imagePath.extension().string());
    return result;
}

}
