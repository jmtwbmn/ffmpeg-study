cmake_minimum_required(VERSION 3.10)
project(Playermyself)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 查找依赖库 (FFmpeg 和 SDL2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample)
pkg_check_modules(SDL2 REQUIRED sdl2)

# 收集 src 目录下所有的 cpp 文件
file(GLOB SOURCES "src/*.cpp")

# 生成可执行文件
add_executable(PlayerProject ${SOURCES})

# 链接库
target_link_libraries(PlayerProject PRIVATE ${FFMPEG_LIBRARIES} ${SDL2_LIBRARIES})
target_include_directories(PlayerProject PRIVATE ${FFMPEG_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})