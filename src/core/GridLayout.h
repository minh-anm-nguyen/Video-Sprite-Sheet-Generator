#pragma once

#include "core/Rect.h"

namespace vss {

class GridLayout {
public:
    GridLayout(int cols, int rows, int cellWidth, double aspectRatio);

    int cols() const noexcept;
    int rows() const noexcept;
    int cellWidth() const noexcept;
    int cellHeight() const noexcept;
    int capacity() const noexcept;
    int canvasWidth() const noexcept;
    int canvasHeight() const noexcept;
    Rect cellRect(int index) const;
    GridLayout fitToContent(int n) const;

private:
    GridLayout(int cols, int rows, int cellWidth, int cellHeight) noexcept;

    int cols_;
    int rows_;
    int cellWidth_;
    int cellHeight_;
};

}
