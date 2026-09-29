cmake_minimum_required(VERSION 3.20)

project(OpenDoorsPackageConsumer LANGUAGES C CXX)

include(CTest)

set(OPENDOORS_VARIANT "Shared" CACHE STRING
    "Installed OpenDoors target to test (Shared, Static, or StaticMT)")
set_property(CACHE OPENDOORS_VARIANT PROPERTY STRINGS Shared Static StaticMT)

if(NOT OPENDOORS_VARIANT STREQUAL "Shared" AND
        NOT OPENDOORS_VARIANT STREQUAL "Static" AND
        NOT OPENDOORS_VARIANT STREQUAL "StaticMT")
    message(FATAL_ERROR
        "OPENDOORS_VARIANT must be Shared, Static, or StaticMT")
endif()

find_package(OpenDoors 6.3 CONFIG REQUIRED COMPONENTS ${OPENDOORS_VARIANT})

add_executable(opendoors_package_consumer
    ../link_smoke.c
    personality_header_smoke.cpp)
target_link_libraries(opendoors_package_consumer
    PRIVATE OpenDoors::${OPENDOORS_VARIANT})
add_test(NAME package_consumer COMMAND opendoors_package_consumer)
