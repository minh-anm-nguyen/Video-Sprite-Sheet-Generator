#pragma once

#include <filesystem>
#include <optional>

namespace vss {

enum class ImageFormat {
    Jpeg,
    Png
};

std::optional<ImageFormat> formatFromExtension(const std::filesystem::path& path);

}
