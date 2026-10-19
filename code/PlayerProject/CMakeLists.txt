cmake_minimum_required(VERSION 3.16)

project(PlayerProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavutil
    libswscale
    libswresample
)

pkg_check_modules(SDL2 REQUIRED sdl2)

include_directories(
    ${FFMPEG_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)

link_directories(
    ${FFMPEG_LIBRARY_DIRS}
    ${SDL2_LIBRARY_DIRS}
)

add_executable(PlayerProject
    src/main.cpp
    src/Player.cpp
    src/Decoder.cpp
    src/AudioPlayer.cpp
    src/VideoRenderer.cpp
)

target_link_libraries(PlayerProject
    ${FFMPEG_LIBRARIES}
    ${SDL2_LIBRARIES}
    pthread
)
