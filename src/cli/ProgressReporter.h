#pragma once

#include <chrono>
#include <string>

namespace vss {

class ProgressReporter {
public:
    ProgressReporter(int total, bool quiet);
    ~ProgressReporter();
    ProgressReporter(const ProgressReporter&) = delete;
    ProgressReporter& operator=(const ProgressReporter&) = delete;

    static std::string formatBar(int completed, int total, double elapsedSec);
    void update(int completed);
    void finish();

private:
    int total_;
    bool active_;
    bool finished_ = false;
    std::chrono::steady_clock::time_point start_;
};

}
