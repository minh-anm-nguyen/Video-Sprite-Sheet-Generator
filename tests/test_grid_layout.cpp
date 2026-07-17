#include <catch2/catch_test_macros.hpp>

#include "core/GridLayout.h"
#include "core/Rect.h"

using namespace vss;

namespace {

constexpr double kAspect16x9 = 16.0 / 9.0;

}

TEST_CASE("cellHeight follows the display aspect ratio") {
    CHECK(GridLayout(5, 5, 160, kAspect16x9).cellHeight() == 90);
    CHECK(GridLayout(5, 5, 160, 9.0 / 16.0).cellHeight() == 284);
    CHECK(GridLayout(5, 5, 160, 4.0 / 3.0).cellHeight() == 120);
}

TEST_CASE("cellHeight is rounded to the nearest even value with a floor of two") {
    CHECK(GridLayout(5, 5, 162, kAspect16x9).cellHeight() == 92);
    CHECK(GridLayout(5, 5, 160, 1000.0).cellHeight() == 2);
    CHECK(GridLayout(5, 5, 160, kAspect16x9).cellHeight() % 2 == 0);
    CHECK(GridLayout(5, 5, 160, 9.0 / 16.0).cellHeight() % 2 == 0);
}

TEST_CASE("capacity and canvas size derive from the grid") {
    GridLayout grid(5, 5, 160, kAspect16x9);
    CHECK(grid.capacity() == 25);
    CHECK(grid.canvasWidth() == 800);
    CHECK(grid.canvasHeight() == 450);
}

TEST_CASE("cellRect walks left to right then top to bottom") {
    GridLayout grid(5, 5, 160, kAspect16x9);
    CHECK(grid.cellRect(0) == Rect{0, 0, 160, 90});
    CHECK(grid.cellRect(1) == Rect{160, 0, 160, 90});
    CHECK(grid.cellRect(4) == Rect{640, 0, 160, 90});
    CHECK(grid.cellRect(5) == Rect{0, 90, 160, 90});
    CHECK(grid.cellRect(7) == Rect{320, 90, 160, 90});
    CHECK(grid.cellRect(24) == Rect{640, 360, 160, 90});
}

TEST_CASE("fitToContent shrinks the last sheet to its content") {
    GridLayout grid(5, 5, 160, kAspect16x9);

    GridLayout n3 = grid.fitToContent(3);
    CHECK(n3.cols() == 3);
    CHECK(n3.rows() == 1);

    GridLayout n5 = grid.fitToContent(5);
    CHECK(n5.cols() == 5);
    CHECK(n5.rows() == 1);

    GridLayout n8 = grid.fitToContent(8);
    CHECK(n8.cols() == 5);
    CHECK(n8.rows() == 2);

    GridLayout n25 = grid.fitToContent(25);
    CHECK(n25.cols() == 5);
    CHECK(n25.rows() == 5);

    GridLayout n26 = grid.fitToContent(26);
    CHECK(n26.cols() == 5);
    CHECK(n26.rows() == 5);
}

TEST_CASE("fitToContent handles the single thumbnail case") {
    GridLayout grid(5, 5, 160, kAspect16x9);
    GridLayout n1 = grid.fitToContent(1);
    CHECK(n1.cols() == 1);
    CHECK(n1.rows() == 1);
    CHECK(n1.capacity() == 1);
    CHECK(n1.canvasWidth() == 160);
    CHECK(n1.canvasHeight() == 90);
}

TEST_CASE("fitToContent keeps existing cell coordinates unchanged") {
    GridLayout grid(5, 5, 160, kAspect16x9);
    GridLayout fitted = grid.fitToContent(8);
    for (int i = 0; i < 8; ++i) {
        CHECK(fitted.cellRect(i) == grid.cellRect(i));
    }
}
