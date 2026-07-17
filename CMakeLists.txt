cmake_minimum_required(VERSION 3.21)

project(video-sprite-sheet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(FFMPEG REQUIRED)

add_executable(video-sprite-sheet
    src/main.cpp
)

target_include_directories(video-sprite-sheet PRIVATE src)
target_include_directories(video-sprite-sheet SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(video-sprite-sheet PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(video-sprite-sheet PRIVATE ${FFMPEG_LIBRARIES})

if(MSVC)
    target_compile_options(video-sprite-sheet PRIVATE /W4)
else()
    target_compile_options(video-sprite-sheet PRIVATE -Wall -Wextra -Wpedantic)
endif()
