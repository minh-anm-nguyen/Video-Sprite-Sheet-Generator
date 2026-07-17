#pragma once

namespace vss {

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    friend bool operator==(const Rect&, const Rect&) = default;
};

}
