find_package(Catch2 3 REQUIRED)

add_executable(vss_tests
    test_app_config.cpp
    test_atomic_file_writer.cpp
    test_cli_parser.cpp
    test_core_types.cpp
    test_grid_layout.cpp
    test_image_format.cpp
    test_pixel_pipeline.cpp
    test_timestamp_planner.cpp
    test_video_file.cpp
    test_vtt.cpp
)
target_include_directories(vss_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vss_tests SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_compile_definitions(vss_tests PRIVATE
    VSS_TEST_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_link_libraries(vss_tests PRIVATE vss_core vss_media Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(vss_tests)
