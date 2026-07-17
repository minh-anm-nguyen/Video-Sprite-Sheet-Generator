find_package(Catch2 3 REQUIRED)

add_executable(vss_tests
    test_core_types.cpp
    test_grid_layout.cpp
    test_timestamp_planner.cpp
)
target_link_libraries(vss_tests PRIVATE vss_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(vss_tests)
