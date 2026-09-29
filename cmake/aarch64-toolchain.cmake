# Cross-compilation toolchain for a 64-bit Raspberry Pi (Pi 3 and up,
# 64-bit Raspberry Pi OS) build from this x86_64 Linux dev machine.
# Usage:
#   cmake -S . -B build-pi -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-toolchain.cmake
#   cmake --build build-pi --config Release -j4

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
