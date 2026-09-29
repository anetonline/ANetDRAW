cmake_minimum_required(VERSION 3.20)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" OPENDOORS_VERSION
    LIMIT_COUNT 1)
project(OpenDoors VERSION "${OPENDOORS_VERSION}" LANGUAGES C)

set(_opendoors_testing_explicit OFF)
if(DEFINED BUILD_TESTING)
    set(_opendoors_testing_explicit ON)
endif()
if(NOT _opendoors_testing_explicit)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND)
        set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(OPENDOORS_BUILD_SHARED "Build the OpenDoors shared library" ON)
option(OPENDOORS_BUILD_STATIC "Build the OpenDoors static library" ON)
option(OPENDOORS_BUILD_MSVC_STATIC_MT
    "Build an additional MSVC static library using the static CRT" OFF)
option(OPENDOORS_BUILD_EXAMPLES "Build the OpenDoors example programs" ON)
option(OPENDOORS_WARNINGS_AS_ERRORS
    "Treat OpenDoors compiler and linker warnings as errors" OFF)

add_subdirectory(src)

if(OPENDOORS_BUILD_SHARED)
    set(OPENDOORS_DEFAULT_LINK_TARGET OpenDoors::Shared)
else()
    set(OPENDOORS_DEFAULT_LINK_TARGET OpenDoors::Static)
endif()

if(OPENDOORS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

configure_package_config_file(
    cmake/OpenDoorsConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/OpenDoorsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenDoors)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OpenDoorsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OpenDoorsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenDoorsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenDoors)
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/OpenDoors)
