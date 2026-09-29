cmake_minimum_required(VERSION 3.20)
project(anetdraw C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# OpenDoors builds its own shared+static libs and examples/tests by
# default -- ANetDRAW only needs the static lib linked in. Same
# CACHE...FORCE pattern rdq3_opendoors/ANetCHESS/ANetCRAFT use.
set(OPENDOORS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(OPENDOORS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/OpenDoors)

# OpenDoors distinguishes Windows GUI vs. console executables at link
# time (OpenDoors::Static vs. OpenDoors::StaticConsole). Without
# StaticConsole on Windows, od_parse_cmd_line() refuses to run under a
# console host ("requires a Windows GUI executable") -- confirmed live
# on RDQ3/ANetCHESS via a real Wine run, not just a clean cross-compile.
# ANetDRAW is a plain console door on every platform, same as those.
if(WIN32)
    set(OPENDOORS_LINK_TARGET OpenDoors::StaticConsole)
else()
    set(OPENDOORS_LINK_TARGET OpenDoors::Static)
endif()

# Explicit file list, not a glob -- a stray file can never silently get
# pulled in or left out. Grows one entry per build phase.
add_executable(anetdraw
    src/main.c
    src/door_init.c
    src/output.c
    src/canvas.c
    src/render.c
    src/fkeys.c
    src/input.c
    src/editor.c
    src/tools.c
    src/undo.c
    src/fileio.c
    src/files.c
    src/splash.c
    src/splash_art.c
    src/zmodem.c
    src/transfer.c
    src/gallery.c
    src/tdf.c
    src/formats.c
    src/imgimport.c
    src/wall.c
    src/font8x16.c
)
target_link_libraries(anetdraw PRIVATE ${OPENDOORS_LINK_TARGET})

# Windows local mode's own window (src/wingui.c) and its IBM VGA font
if(WIN32)
    target_sources(anetdraw PRIVATE src/wingui.c)
    target_link_libraries(anetdraw PRIVATE user32 gdi32)
endif()
