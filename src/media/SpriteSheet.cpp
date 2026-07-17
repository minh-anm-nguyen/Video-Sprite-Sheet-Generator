#include "media/SpriteSheet.h"

#include <cassert>
#include <cstring>

extern "C" {
#include <libavutil/frame.h>
}

namespace vss {

SpriteSheet::SpriteSheet(const GridLayout& layout, int index)
    : width_(layout.canvasWidth()),
      height_(layout.canvasHeight()),
      index_(index),
      canvas_(static_cast<std::size_t>(layout.canvasWidth()) * layout.canvasHeight() * 3,
              0) {}

void SpriteSheet::place(const Frame& thumbnail, const Rect& rect) {
    const AVFrame* src = thumbnail.handle();
    assert(src != nullptr);
    assert(src->width == rect.w);
    assert(src->height == rect.h);
    assert(rect.x >= 0 && rect.y >= 0);
    assert(rect.x + rect.w <= width_ && rect.y + rect.h <= height_);
    for (int y = 0; y < rect.h; ++y) {
        std::memcpy(canvas_.data() + (static_cast<std::size_t>(rect.y + y) * width_ + rect.x) * 3,
                    src->data[0] + static_cast<std::size_t>(y) * src->linesize[0],
                    static_cast<std::size_t>(rect.w) * 3);
    }
}

void SpriteSheet::fillBlack(const Rect& rect) {
    assert(rect.x >= 0 && rect.y >= 0);
    assert(rect.x + rect.w <= width_ && rect.y + rect.h <= height_);
    for (int y = 0; y < rect.h; ++y) {
        std::memset(canvas_.data() + (static_cast<std::size_t>(rect.y + y) * width_ + rect.x) * 3,
                    0, static_cast<std::size_t>(rect.w) * 3);
    }
}

int SpriteSheet::width() const noexcept {
    return width_;
}

int SpriteSheet::height() const noexcept {
    return height_;
}

int SpriteSheet::stride() const noexcept {
    return width_ * 3;
}

int SpriteSheet::index() const noexcept {
    return index_;
}

const std::uint8_t* SpriteSheet::data() const noexcept {
    return canvas_.data();
}

}
