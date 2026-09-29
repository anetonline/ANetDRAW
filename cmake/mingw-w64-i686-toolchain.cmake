# 32-bit Windows cross-compilation toolchain -- for older/32-bit
# Windows installs (the 64-bit build produces a binary Windows itself
# refuses to run there: "The version of this file is not compatible
# with the version of Windows you're running"). Usage:
#   cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-i686-toolchain.cmake
#   cmake --build build-win32 --config Release -j4

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
