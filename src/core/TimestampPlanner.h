#pragma once

#include <vector>

namespace vss {

class TimestampPlanner {
public:
    static std::vector<double> compute(double intervalSec, double durationSec);
};

}
