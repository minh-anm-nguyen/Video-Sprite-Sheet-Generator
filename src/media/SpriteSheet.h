#pragma once

#include <cstdint>
#include <vector>

#include "core/GridLayout.h"
#include "core/Rect.h"
#include "media/Frame.h"

namespace vss {

class SpriteSheet {
public:
    SpriteSheet(const GridLayout& layout, int index);

    void place(const Frame& thumbnail, const Rect& rect);
    void fillBlack(const Rect& rect);

    int width() const noexcept;
    int height() const noexcept;
    int stride() const noexcept;
    int index() const noexcept;
    const std::uint8_t* data() const noexcept;

private:
    int width_;
    int height_;
    int index_;
    std::vector<std::uint8_t> canvas_;
};

}
