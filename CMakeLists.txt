cmake_minimum_required(VERSION 3.21)

project(video-sprite-sheet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(FFMPEG REQUIRED)

add_library(vss_core STATIC
    src/cli/CliParser.cpp
    src/core/AppConfig.cpp
    src/core/AppError.cpp
    src/core/GridLayout.cpp
    src/core/TimestampPlanner.cpp
    src/media/ImageFormat.cpp
    src/output/VttCueBuilder.cpp
    src/output/WebVttWriter.cpp
)
target_include_directories(vss_core PUBLIC src)

add_library(vss_media STATIC
    src/media/Frame.cpp
)
target_include_directories(vss_media SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(vss_media PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(vss_media PUBLIC vss_core ${FFMPEG_LIBRARIES})

add_executable(video-sprite-sheet
    src/main.cpp
)
target_link_libraries(video-sprite-sheet PRIVATE vss_media)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
