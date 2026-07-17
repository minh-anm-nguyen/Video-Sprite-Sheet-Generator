#pragma once

#include <atomic>

namespace vss {

class CancellationToken {
public:
    void requestCancel() noexcept {
        cancelled_.store(true, std::memory_order_relaxed);
    }

    bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<bool> cancelled_{false};
};

}
