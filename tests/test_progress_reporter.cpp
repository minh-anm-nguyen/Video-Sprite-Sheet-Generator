#include <catch2/catch_test_macros.hpp>

#include "cli/ProgressReporter.h"
#include "core/CancellationToken.h"

using namespace vss;

TEST_CASE("formatBar renders an empty bar before any work") {
    CHECK(ProgressReporter::formatBar(0, 25, 0.0) ==
          "[--------------------] 0/25 0% ETA --");
}

TEST_CASE("formatBar renders progress percent and eta") {
    CHECK(ProgressReporter::formatBar(12, 25, 12.0) ==
          "[#########-----------] 12/25 48% ETA 13s");
}

TEST_CASE("formatBar renders a full bar at completion") {
    CHECK(ProgressReporter::formatBar(25, 25, 30.0) ==
          "[####################] 25/25 100% ETA 0s");
}

TEST_CASE("a cancellation token starts clear and latches") {
    CancellationToken token;
    CHECK_FALSE(token.isCancelled());
    token.requestCancel();
    CHECK(token.isCancelled());
    token.requestCancel();
    CHECK(token.isCancelled());
}
