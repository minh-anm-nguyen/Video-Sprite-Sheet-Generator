#include "cli/ProgressReporter.h"

#include <cstdio>

#include "platform/Platform.h"

namespace vss {

ProgressReporter::ProgressReporter(int total, bool quiet)
    : total_(total),
      active_(!quiet && total > 0 && platform::stdoutIsTerminal()),
      start_(std::chrono::steady_clock::now()) {}

ProgressReporter::~ProgressReporter() {
    if (active_ && !finished_) {
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }
}

std::string ProgressReporter::formatBar(int completed, int total, double elapsedSec) {
    int percent = total > 0 ? completed * 100 / total : 100;
    int filled = total > 0 ? completed * 20 / total : 20;
    std::string bar(static_cast<std::size_t>(filled), '#');
    bar.append(static_cast<std::size_t>(20 - filled), '-');
    std::string eta = "--";
    if (completed >= total) {
        eta = "0s";
    } else if (completed > 0) {
        long long remaining = static_cast<long long>(
            elapsedSec * (total - completed) / completed + 0.5);
        eta = std::to_string(remaining) + "s";
    }
    return "[" + bar + "] " + std::to_string(completed) + "/" + std::to_string(total) +
           " " + std::to_string(percent) + "% ETA " + eta;
}

void ProgressReporter::update(int completed) {
    if (!active_) {
        return;
    }
    double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    std::string line = "\r" + formatBar(completed, total_, elapsed) + "   ";
    std::fputs(line.c_str(), stdout);
    std::fflush(stdout);
}

void ProgressReporter::finish() {
    if (active_ && !finished_) {
        update(total_);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }
    finished_ = true;
}

}
