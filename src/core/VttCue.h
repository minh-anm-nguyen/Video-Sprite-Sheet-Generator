#pragma once

#include <string>

#include "core/Rect.h"

namespace vss {

struct VttCue {
    double start = 0.0;
    double end = 0.0;
    std::string imageRef;
    Rect region;

    friend bool operator==(const VttCue&, const VttCue&) = default;
};

}
