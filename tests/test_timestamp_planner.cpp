#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/AppError.h"
#include "core/TimestampPlanner.h"

using namespace vss;

namespace {

ErrorCategory categoryOf(double interval, double duration) {
    try {
        TimestampPlanner::compute(interval, duration);
    } catch (const AppError& e) {
        return e.category();
    }
    return ErrorCategory::Processing;
}

}

TEST_CASE("auto interval fills exactly one 5x5 sheet") {
    std::vector<double> ts = TimestampPlanner::compute(4.0, 100.0);
    REQUIRE(ts.size() == 25);
    CHECK(ts.front() == 0.0);
    CHECK(ts.back() == 96.0);
}

TEST_CASE("marks stop strictly before the duration") {
    std::vector<double> ts = TimestampPlanner::compute(10.0, 35.0);
    CHECK(ts == std::vector<double>{0.0, 10.0, 20.0, 30.0});
}

TEST_CASE("a video shorter than the interval still yields the first frame") {
    CHECK(TimestampPlanner::compute(10.0, 5.0) == std::vector<double>{0.0});
}

TEST_CASE("a duration equal to the interval yields a single mark") {
    CHECK(TimestampPlanner::compute(5.0, 5.0) == std::vector<double>{0.0});
}

TEST_CASE("a zero duration still yields the first frame") {
    CHECK(TimestampPlanner::compute(5.0, 0.0) == std::vector<double>{0.0});
}

TEST_CASE("a non positive interval is an argument error") {
    CHECK_THROWS_AS(TimestampPlanner::compute(0.0, 100.0), AppError);
    CHECK_THROWS_AS(TimestampPlanner::compute(-1.0, 100.0), AppError);
    CHECK(categoryOf(0.0, 100.0) == ErrorCategory::Argument);
}

TEST_CASE("more than 10000 marks is an argument error") {
    CHECK_THROWS_AS(TimestampPlanner::compute(0.5, 5000.5), AppError);
    CHECK(categoryOf(0.5, 5000.5) == ErrorCategory::Argument);
}

TEST_CASE("exactly 10000 marks is allowed and multiplication keeps marks drift free") {
    std::vector<double> ts = TimestampPlanner::compute(0.5, 5000.0);
    REQUIRE(ts.size() == 10000);
    CHECK(ts[9999] == 9999 * 0.5);
    CHECK(ts.back() < 5000.0);
}
