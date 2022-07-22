cmake_minimum_required(VERSION 3.22)
project(CHIP_8_Emulator)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_DIR "G:/libs/SDL2-2.0.22")

find_package(SDL2 REQUIRED)
include_directories(CHip_8_Emulator ${SDL2_INCLUDE_DIRS})

add_executable(CHIP_8_Emulator main.cpp chip8.cpp chip8.h)
target_link_libraries(CHIP_8_Emulator ${SDL2_LIBRARIES})
