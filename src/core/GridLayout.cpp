#include "core/GridLayout.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace vss {

GridLayout::GridLayout(int cols, int rows, int cellWidth, double aspectRatio)
    : cols_(cols), rows_(rows), cellWidth_(cellWidth) {
    assert(cols > 0);
    assert(rows > 0);
    assert(cellWidth > 0);
    assert(aspectRatio > 0.0);
    long halfRounded = std::lround(cellWidth / aspectRatio / 2.0);
    cellHeight_ = std::max(2, static_cast<int>(halfRounded * 2));
}

GridLayout::GridLayout(int cols, int rows, int cellWidth, int cellHeight) noexcept
    : cols_(cols), rows_(rows), cellWidth_(cellWidth), cellHeight_(cellHeight) {}

int GridLayout::cols() const noexcept {
    return cols_;
}

int GridLayout::rows() const noexcept {
    return rows_;
}

int GridLayout::cellWidth() const noexcept {
    return cellWidth_;
}

int GridLayout::cellHeight() const noexcept {
    return cellHeight_;
}

int GridLayout::capacity() const noexcept {
    return cols_ * rows_;
}

int GridLayout::canvasWidth() const noexcept {
    return cols_ * cellWidth_;
}

int GridLayout::canvasHeight() const noexcept {
    return rows_ * cellHeight_;
}

Rect GridLayout::cellRect(int index) const {
    assert(index >= 0);
    assert(index < capacity());
    return Rect{(index % cols_) * cellWidth_,
                (index / cols_) * cellHeight_,
                cellWidth_,
                cellHeight_};
}

GridLayout GridLayout::fitToContent(int n) const {
    assert(n > 0);
    if (n >= capacity()) {
        return *this;
    }
    int fittedCols = std::min(n, cols_);
    int fittedRows = (n + cols_ - 1) / cols_;
    return GridLayout(fittedCols, fittedRows, cellWidth_, cellHeight_);
}

}
