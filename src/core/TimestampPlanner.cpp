#include "core/TimestampPlanner.h"

#include <string>

#include "core/AppError.h"

namespace vss {

namespace {

constexpr double kMaxTimestamps = 10000.0;

}

std::vector<double> TimestampPlanner::compute(double intervalSec, double durationSec) {
    if (intervalSec <= 0.0) {
        throw AppError(ErrorCategory::Argument,
                       "interval must be positive, got " + std::to_string(intervalSec));
    }
    if (durationSec / intervalSec > kMaxTimestamps) {
        throw AppError(ErrorCategory::Argument,
                       "interval " + std::to_string(intervalSec) +
                           "s would produce more than 10000 thumbnails for a " +
                           std::to_string(durationSec) + "s video");
    }
    std::vector<double> timestamps;
    for (int k = 0;; ++k) {
        double t = k * intervalSec;
        if (t >= durationSec) {
            break;
        }
        timestamps.push_back(t);
    }
    if (timestamps.empty()) {
        timestamps.push_back(0.0);
    }
    return timestamps;
}

}
