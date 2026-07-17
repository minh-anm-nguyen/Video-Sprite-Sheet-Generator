#include "media/ImageFormat.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace vss {

std::optional<ImageFormat> formatFromExtension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".jpg" || ext == ".jpeg") {
        return ImageFormat::Jpeg;
    }
    if (ext == ".png") {
        return ImageFormat::Png;
    }
    return std::nullopt;
}

}
