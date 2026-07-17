#pragma once

#include <cstdint>
#include <vector>

#include "media/ImageFormat.h"
#include "media/SpriteSheet.h"

namespace vss {

class ImageEncoder {
public:
    ImageEncoder(ImageFormat format, int quality);

    std::vector<std::uint8_t> encode(const SpriteSheet& sheet) const;

private:
    ImageFormat format_;
    int quality_;
};

}
