cmake_minimum_required(VERSION 3.20)

project(OpenDoorsDOS LANGUAGES C)

option(OPENDOORS_WARNINGS_AS_ERRORS
    "Treat OpenDoors compiler and linker warnings as errors" OFF)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "DOS")
    message(FATAL_ERROR
        "This project is only for DOS cross-builds; set CMAKE_SYSTEM_NAME=DOS")
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom")
    message(FATAL_ERROR "The DOS build requires Open Watcom")
endif()

set(OPENDOORS_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(OPENDOORS_SOURCE_DIR "${OPENDOORS_ROOT}/src")
set(OPENDOORS_INCLUDE_DIR "${OPENDOORS_ROOT}/include")
set(OPENDOORS_EXAMPLES_DIR "${OPENDOORS_ROOT}/examples")
configure_file("${OPENDOORS_EXAMPLES_DIR}/data/door.cfg"
    "${CMAKE_BINARY_DIR}/door.cfg" COPYONLY)
configure_file("${OPENDOORS_EXAMPLES_DIR}/data/DORINFO1.DEF"
    "${CMAKE_BINARY_DIR}/DORINFO1.DEF" COPYONLY)
string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" OPENDOORS_DOS_PROCESSOR)

if(NOT OPENDOORS_DOS_PROCESSOR STREQUAL "I86" AND
   NOT OPENDOORS_DOS_PROCESSOR STREQUAL "I386")
    message(FATAL_ERROR
        "The DOS build supports CMAKE_SYSTEM_PROCESSOR=I86 or I386")
endif()

if(OPENDOORS_DOS_PROCESSOR STREQUAL "I386")
    set(OPENDOORS_DOS32_EXTENDER "DOS4GW" CACHE STRING
        "DOS extender used to link 32-bit executables (DOS4GW or DOS32A)")
    set_property(CACHE OPENDOORS_DOS32_EXTENDER PROPERTY STRINGS
        DOS4GW DOS32A)
    string(TOUPPER "${OPENDOORS_DOS32_EXTENDER}"
        OPENDOORS_DOS32_EXTENDER)
    if(OPENDOORS_DOS32_EXTENDER STREQUAL "DOS32A")
        if(NOT CMAKE_EXE_LINKER_FLAGS MATCHES "system dos4g")
            message(FATAL_ERROR
                "Unable to select Open Watcom's DOS32A linker system")
        endif()
        string(REPLACE "system dos4g" "system dos32x"
            CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
        set(OPENDOORS_DOS32A_STUB "$ENV{WATCOM}/binw/dos32a.exe")
        if(NOT EXISTS "${OPENDOORS_DOS32A_STUB}")
            message(FATAL_ERROR
                "Cannot find the DOS32A extender at ${OPENDOORS_DOS32A_STUB}")
        endif()
        configure_file("${OPENDOORS_DOS32A_STUB}"
            "${CMAKE_BINARY_DIR}/dos32a.exe" COPYONLY)
    elseif(NOT OPENDOORS_DOS32_EXTENDER STREQUAL "DOS4GW")
        message(FATAL_ERROR
            "OPENDOORS_DOS32_EXTENDER must be DOS4GW or DOS32A")
    endif()
endif()

set(OPENDOORS_DOS_SOURCES
    ODAuto.c
    ODBlock.c
    ODCFile.c
    ODCmdLn.c
    ODCom.c
    ODCore.c
    ODDrBox.c
    ODEdit.c
    ODEdStr.c
    ODEmu.c
    ODFormat.c
    ODFmtFB.c
    ODGetIn.c
    ODGraph.c
    ODInEx1.c
    ODInEx2.c
    ODInQue.c
    ODKrnl.c
    ODList.c
    ODLog.c
    ODMulti.c
    ODPlat.c
    ODPCB.c
    ODPopup.c
    ODPrntf.c
    ODRA.c
    ODRsv.c
    ODSafe.c
    ODScrn.c
    ODSpawn.c
    ODStand.c
    ODStat.c
    ODSync.c
    ODUtil.c
    ODWCat.c
    ODWin.c
)
list(TRANSFORM OPENDOORS_DOS_SOURCES PREPEND "${OPENDOORS_SOURCE_DIR}/")

# Keep example-only inputs separate from OPENDOORS_DOS_SOURCES.  Construct
# their paths from names so the strict library-source manifest checker does
# not mistake them for library inputs.
set(OPENDOORS_VOTE_EXAMPLE_SOURCES)
foreach(vote_source IN ITEMS ex_vote.c ex_votio.c)
    list(APPEND OPENDOORS_VOTE_EXAMPLE_SOURCES
        "${OPENDOORS_EXAMPLES_DIR}/${vote_source}")
endforeach()
list(GET OPENDOORS_VOTE_EXAMPLE_SOURCES 1 OPENDOORS_VOTE_IO_SOURCE)

if(OPENDOORS_DOS_PROCESSOR STREQUAL "I386")
    list(APPEND OPENDOORS_DOS_SOURCES
        "${OPENDOORS_SOURCE_DIR}/OD32DPMI.c"
        "${OPENDOORS_SOURCE_DIR}/OD32Foss.c")
endif()

if(OPENDOORS_DOS_PROCESSOR STREQUAL "I86")
find_program(OPENDOORS_WASM wasm REQUIRED)
set(OPENDOORS_SWAP_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/ODSwap.obj")
add_custom_command(
    OUTPUT "${OPENDOORS_SWAP_OBJECT}"
    COMMAND "${OPENDOORS_WASM}" -q -dLCODE -dLDATA
        "-fo=${OPENDOORS_SWAP_OBJECT}" "${OPENDOORS_SOURCE_DIR}/ODSwap.asm"
    DEPENDS "${OPENDOORS_SOURCE_DIR}/ODSwap.asm"
    VERBATIM)
set_source_files_properties("${OPENDOORS_SWAP_OBJECT}" PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE)

add_library(opendoors_dos STATIC
    ${OPENDOORS_DOS_SOURCES}
    "${OPENDOORS_SWAP_OBJECT}")
target_include_directories(opendoors_dos PUBLIC
    "${OPENDOORS_INCLUDE_DIR}" "${OPENDOORS_SOURCE_DIR}"
    PRIVATE "${OPENDOORS_ROOT}")
target_compile_options(opendoors_dos PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(opendoors_dos PRIVATE -we)
endif()
target_compile_definitions(opendoors_dos PRIVATE OPENDOORS_HAVE_VSNPRINTF=1)
set_target_properties(opendoors_dos PROPERTIES
    OUTPUT_NAME ODoorl
    PREFIX "")

set(OPENDOORS_PERSONALITY_SDK_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/personality-sdk")
file(MAKE_DIRECTORY "${OPENDOORS_PERSONALITY_SDK_DIR}")
foreach(personality_sdk_file IN ITEMS
    include/OpenDoor.h
    src/ODSync.h
    include/ODStat.h
    src/ODTypes.h
    src/ODStand.c
    src/ODPCB.c
    src/ODRA.c
    src/ODWCat.c
    tests/dos_personality_smoke.c)
    get_filename_component(personality_sdk_name
        "${personality_sdk_file}" NAME)
    configure_file(
        "${OPENDOORS_ROOT}/${personality_sdk_file}"
        "${OPENDOORS_PERSONALITY_SDK_DIR}/${personality_sdk_name}"
        COPYONLY)
endforeach()

add_library(dos_personality_sdk_compile OBJECT
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODStand.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODPCB.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODRA.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODWCat.c")
target_include_directories(dos_personality_sdk_compile PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")
target_compile_options(dos_personality_sdk_compile PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_personality_sdk_compile PRIVATE -we)
endif()
target_compile_definitions(dos_personality_sdk_compile PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)

add_executable(dos_link_smoke "${OPENDOORS_ROOT}/tests/link_smoke.c")
target_compile_options(dos_link_smoke PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_link_smoke PRIVATE -we)
endif()
target_compile_definitions(dos_link_smoke PRIVATE OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_link_smoke PRIVATE opendoors_dos)

foreach(example IN ITEMS ex_chat ex_diag ex_hello ex_music ex_ski)
    add_executable(${example} "${OPENDOORS_EXAMPLES_DIR}/${example}.c")
    target_compile_options(${example} PRIVATE -ml -zq -za99)
    if(OPENDOORS_WARNINGS_AS_ERRORS)
        target_compile_options(${example} PRIVATE -we)
    endif()
    target_compile_definitions(${example} PRIVATE
        OPENDOORS_HAVE_VSNPRINTF=1)
    target_link_libraries(${example} PRIVATE opendoors_dos)
endforeach()

add_executable(ex_vote ${OPENDOORS_VOTE_EXAMPLE_SOURCES})
target_compile_options(ex_vote PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(ex_vote PRIVATE -we)
endif()
target_compile_definitions(ex_vote PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1 MULTINODE_AWARE)
target_link_libraries(ex_vote PRIVATE opendoors_dos)

add_executable(vote_io_test
    "${OPENDOORS_ROOT}/tests/vote_io_test.c"
    "${OPENDOORS_VOTE_IO_SOURCE}")
set_target_properties(vote_io_test PROPERTIES OUTPUT_NAME odvtest)
target_include_directories(vote_io_test PRIVATE
    "${OPENDOORS_INCLUDE_DIR}" "${OPENDOORS_EXAMPLES_DIR}")
target_compile_options(vote_io_test PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(vote_io_test PRIVATE -we)
endif()

add_executable(dos_runtime_smoke
    "${OPENDOORS_ROOT}/tests/dos_runtime_smoke.c")
set_target_properties(dos_runtime_smoke PROPERTIES OUTPUT_NAME odtest)
target_compile_options(dos_runtime_smoke PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_runtime_smoke PRIVATE -we)
endif()
target_compile_definitions(dos_runtime_smoke PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_runtime_smoke PRIVATE opendoors_dos)

add_executable(dos_personality_smoke
    "${OPENDOORS_PERSONALITY_SDK_DIR}/dos_personality_smoke.c")
set_target_properties(dos_personality_smoke PROPERTIES OUTPUT_NAME odpers)
target_include_directories(dos_personality_smoke PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")
target_compile_options(dos_personality_smoke PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_personality_smoke PRIVATE -we)
endif()
target_compile_definitions(dos_personality_smoke PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_personality_smoke PRIVATE opendoors_dos)

add_executable(dos_acceptance_surface
    "${OPENDOORS_ROOT}/tests/acceptance/public_surface_test.c")
set_target_properties(dos_acceptance_surface PROPERTIES OUTPUT_NAME odapi)
target_include_directories(dos_acceptance_surface PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos_acceptance_surface PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_acceptance_surface PRIVATE -we)
endif()
target_compile_definitions(dos_acceptance_surface PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_acceptance_surface PRIVATE opendoors_dos)

add_executable(dos_acceptance_reservation
    "${OPENDOORS_ROOT}/tests/acceptance/reservation_test.c")
set_target_properties(dos_acceptance_reservation PROPERTIES OUTPUT_NAME odres)
target_include_directories(dos_acceptance_reservation PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos_acceptance_reservation PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_acceptance_reservation PRIVATE -we)
endif()
target_compile_definitions(dos_acceptance_reservation PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_acceptance_reservation PRIVATE opendoors_dos)

add_executable(dos_acceptance_serial
    "${OPENDOORS_ROOT}/tests/acceptance/transport_door.c")
set_target_properties(dos_acceptance_serial PROPERTIES OUTPUT_NAME odserial)
target_include_directories(dos_acceptance_serial PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos_acceptance_serial PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_acceptance_serial PRIVATE -we)
endif()
target_compile_definitions(dos_acceptance_serial PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos_acceptance_serial PRIVATE opendoors_dos)

add_executable(dos_acceptance_fossil
    "${OPENDOORS_ROOT}/tests/acceptance/transport_door.c")
set_target_properties(dos_acceptance_fossil PROPERTIES OUTPUT_NAME odfossil)
target_include_directories(dos_acceptance_fossil PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos_acceptance_fossil PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_acceptance_fossil PRIVATE -we)
endif()
target_compile_definitions(dos_acceptance_fossil PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1 OD_ACCEPTANCE_DOS_FOSSIL=1)
target_link_libraries(dos_acceptance_fossil PRIVATE opendoors_dos)

add_executable(dos_acceptance_modem_answer
    "${OPENDOORS_ROOT}/tests/acceptance/modem_answer.c")
set_target_properties(dos_acceptance_modem_answer PROPERTIES
    OUTPUT_NAME odanswer)
target_compile_options(dos_acceptance_modem_answer PRIVATE -ml -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos_acceptance_modem_answer PRIVATE -we)
endif()

else()

function(opendoors_configure_dos32_target target convention)
    target_include_directories(${target} PUBLIC
        "${OPENDOORS_INCLUDE_DIR}" "${OPENDOORS_SOURCE_DIR}"
        PRIVATE "${OPENDOORS_ROOT}")
    target_compile_options(${target} PRIVATE
        "-${convention}" -zq -za99)
    if(OPENDOORS_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE -we)
    endif()
    target_compile_definitions(${target} PRIVATE
        OPENDOORS_HAVE_VSNPRINTF=1)
endfunction()

add_executable(dos32_acceptance_modem_answer
    "${OPENDOORS_ROOT}/tests/acceptance/modem_answer.c")
set_target_properties(dos32_acceptance_modem_answer PROPERTIES
    OUTPUT_NAME odanswer)
opendoors_configure_dos32_target(dos32_acceptance_modem_answer 3r)

add_library(opendoors_dos32r STATIC ${OPENDOORS_DOS_SOURCES})
opendoors_configure_dos32_target(opendoors_dos32r 3r)
set_target_properties(opendoors_dos32r PROPERTIES
    OUTPUT_NAME ODOOR32R
    PREFIX "")

add_library(opendoors_dos32s STATIC ${OPENDOORS_DOS_SOURCES})
opendoors_configure_dos32_target(opendoors_dos32s 3s)
set_target_properties(opendoors_dos32s PROPERTIES
    OUTPUT_NAME ODOOR32S
    PREFIX "")

set(OPENDOORS_PERSONALITY_SDK_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/personality-sdk")
file(MAKE_DIRECTORY "${OPENDOORS_PERSONALITY_SDK_DIR}")
foreach(personality_sdk_file IN ITEMS
    include/OpenDoor.h
    src/ODSync.h
    include/ODStat.h
    src/ODTypes.h
    src/ODStand.c
    src/ODPCB.c
    src/ODRA.c
    src/ODWCat.c
    tests/dos_personality_smoke.c)
    get_filename_component(personality_sdk_name
        "${personality_sdk_file}" NAME)
    configure_file(
        "${OPENDOORS_ROOT}/${personality_sdk_file}"
        "${OPENDOORS_PERSONALITY_SDK_DIR}/${personality_sdk_name}"
        COPYONLY)
endforeach()

add_library(dos32_personality_sdk_compile_r OBJECT
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODStand.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODPCB.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODRA.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODWCat.c")
opendoors_configure_dos32_target(dos32_personality_sdk_compile_r 3r)
target_include_directories(dos32_personality_sdk_compile_r PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")

add_library(dos32_personality_sdk_compile_s OBJECT
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODStand.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODPCB.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODRA.c"
    "${OPENDOORS_PERSONALITY_SDK_DIR}/ODWCat.c")
opendoors_configure_dos32_target(dos32_personality_sdk_compile_s 3s)
target_include_directories(dos32_personality_sdk_compile_s PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")

add_executable(dos32_link_smoke_r "${OPENDOORS_ROOT}/tests/link_smoke.c")
opendoors_configure_dos32_target(dos32_link_smoke_r 3r)
target_link_libraries(dos32_link_smoke_r PRIVATE opendoors_dos32r)

add_executable(dos32_link_smoke_s "${OPENDOORS_ROOT}/tests/link_smoke.c")
opendoors_configure_dos32_target(dos32_link_smoke_s 3s)
target_link_libraries(dos32_link_smoke_s PRIVATE opendoors_dos32s)

add_executable(dos32_abi_smoke_default
    "${OPENDOORS_ROOT}/tests/dos32_abi_smoke.c")
opendoors_configure_dos32_target(dos32_abi_smoke_default 3r)
target_link_libraries(dos32_abi_smoke_default PRIVATE opendoors_dos32r)

add_executable(dos32_abi_smoke_hostile
    "${OPENDOORS_ROOT}/tests/dos32_abi_smoke.c")
opendoors_configure_dos32_target(dos32_abi_smoke_hostile 3r)
target_compile_options(dos32_abi_smoke_hostile PRIVATE -zp16 -em)
target_link_libraries(dos32_abi_smoke_hostile PRIVATE opendoors_dos32r)

foreach(example IN ITEMS ex_chat ex_diag ex_hello ex_music ex_ski)
    add_executable(${example} "${OPENDOORS_EXAMPLES_DIR}/${example}.c")
    opendoors_configure_dos32_target(${example} 3r)
    target_link_libraries(${example} PRIVATE opendoors_dos32r)
endforeach()

add_executable(ex_vote ${OPENDOORS_VOTE_EXAMPLE_SOURCES})
opendoors_configure_dos32_target(ex_vote 3r)
target_compile_definitions(ex_vote PRIVATE MULTINODE_AWARE)
target_link_libraries(ex_vote PRIVATE opendoors_dos32r)

add_executable(vote_io_test
    "${OPENDOORS_ROOT}/tests/vote_io_test.c"
    "${OPENDOORS_VOTE_IO_SOURCE}")
set_target_properties(vote_io_test PROPERTIES OUTPUT_NAME od32vot)
target_include_directories(vote_io_test PRIVATE
    "${OPENDOORS_INCLUDE_DIR}" "${OPENDOORS_EXAMPLES_DIR}")
opendoors_configure_dos32_target(vote_io_test 3r)

add_executable(dos32_runtime_smoke
    "${OPENDOORS_ROOT}/tests/dos32_runtime_smoke.c")
set_target_properties(dos32_runtime_smoke PROPERTIES OUTPUT_NAME od32test)
opendoors_configure_dos32_target(dos32_runtime_smoke 3r)
target_link_libraries(dos32_runtime_smoke PRIVATE opendoors_dos32r)

add_executable(dos32_spawn_child
    "${OPENDOORS_ROOT}/tests/dos32_spawn_child.c")
set_target_properties(dos32_spawn_child PROPERTIES OUTPUT_NAME OD32CHLD)
opendoors_configure_dos32_target(dos32_spawn_child 3r)
add_dependencies(dos32_runtime_smoke dos32_spawn_child)

add_executable(dos32_runtime_smoke_s
    "${OPENDOORS_ROOT}/tests/dos32_runtime_smoke.c")
set_target_properties(dos32_runtime_smoke_s PROPERTIES OUTPUT_NAME od32tsts)
opendoors_configure_dos32_target(dos32_runtime_smoke_s 3s)
target_link_libraries(dos32_runtime_smoke_s PRIVATE opendoors_dos32s)
add_dependencies(dos32_runtime_smoke_s dos32_spawn_child)

find_program(OPENDOORS_WASM wasm REQUIRED)
find_program(OPENDOORS_WLINK wlink REQUIRED)
set(OPENDOORS_FOSSIL_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/FOSSTSR.obj")
set(OPENDOORS_FOSSIL_TSR "${CMAKE_CURRENT_BINARY_DIR}/FOSSTSR.com")
add_custom_command(
    OUTPUT "${OPENDOORS_FOSSIL_TSR}"
    COMMAND "${OPENDOORS_WASM}" -q -0
        "-fo=${OPENDOORS_FOSSIL_OBJECT}"
        "${OPENDOORS_ROOT}/tests/fossil_tsr.asm"
    COMMAND "${OPENDOORS_WLINK}" format dos com option quiet
        option nodefaultlibs
        name "${OPENDOORS_FOSSIL_TSR}"
        file "${OPENDOORS_FOSSIL_OBJECT}"
    DEPENDS "${OPENDOORS_ROOT}/tests/fossil_tsr.asm"
    VERBATIM)
add_custom_target(dos32_fossil_tsr ALL DEPENDS "${OPENDOORS_FOSSIL_TSR}")

add_executable(dos32_fossil_smoke
    "${OPENDOORS_ROOT}/tests/dos32_fossil_smoke.c")
set_target_properties(dos32_fossil_smoke PROPERTIES OUTPUT_NAME od32foss)
opendoors_configure_dos32_target(dos32_fossil_smoke 3r)
target_link_libraries(dos32_fossil_smoke PRIVATE opendoors_dos32r)

add_executable(dos32_personality_smoke
    "${OPENDOORS_PERSONALITY_SDK_DIR}/dos_personality_smoke.c")
set_target_properties(dos32_personality_smoke PROPERTIES OUTPUT_NAME od32pers)
opendoors_configure_dos32_target(dos32_personality_smoke 3r)
target_include_directories(dos32_personality_smoke PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")
target_link_libraries(dos32_personality_smoke PRIVATE opendoors_dos32r)

add_executable(dos32_personality_smoke_s
    "${OPENDOORS_PERSONALITY_SDK_DIR}/dos_personality_smoke.c")
set_target_properties(dos32_personality_smoke_s PROPERTIES OUTPUT_NAME od32pss)
opendoors_configure_dos32_target(dos32_personality_smoke_s 3s)
target_include_directories(dos32_personality_smoke_s PRIVATE
    "${OPENDOORS_PERSONALITY_SDK_DIR}")
target_link_libraries(dos32_personality_smoke_s PRIVATE opendoors_dos32s)

add_executable(dos32_acceptance_surface_r
    "${OPENDOORS_ROOT}/tests/acceptance/public_surface_test.c")
set_target_properties(dos32_acceptance_surface_r PROPERTIES OUTPUT_NAME odapir)
target_include_directories(dos32_acceptance_surface_r PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos32_acceptance_surface_r PRIVATE -3r -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos32_acceptance_surface_r PRIVATE -we)
endif()
target_compile_definitions(dos32_acceptance_surface_r PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos32_acceptance_surface_r PRIVATE opendoors_dos32r)

add_executable(dos32_acceptance_surface_s
    "${OPENDOORS_ROOT}/tests/acceptance/public_surface_test.c")
set_target_properties(dos32_acceptance_surface_s PROPERTIES OUTPUT_NAME odapis)
target_include_directories(dos32_acceptance_surface_s PRIVATE
    "${OPENDOORS_ROOT}/tests/acceptance")
target_compile_options(dos32_acceptance_surface_s PRIVATE -3s -zq -za99)
if(OPENDOORS_WARNINGS_AS_ERRORS)
    target_compile_options(dos32_acceptance_surface_s PRIVATE -we)
endif()
target_compile_definitions(dos32_acceptance_surface_s PRIVATE
    OPENDOORS_HAVE_VSNPRINTF=1)
target_link_libraries(dos32_acceptance_surface_s PRIVATE opendoors_dos32s)

foreach(convention IN ITEMS r s)
    if(convention STREQUAL "r")
        set(reservation_library opendoors_dos32r)
        set(reservation_switch 3r)
    else()
        set(reservation_library opendoors_dos32s)
        set(reservation_switch 3s)
    endif()
    add_executable(dos32_acceptance_reservation_${convention}
        "${OPENDOORS_ROOT}/tests/acceptance/reservation_test.c")
    set_target_properties(dos32_acceptance_reservation_${convention}
        PROPERTIES OUTPUT_NAME "odres${convention}")
    target_include_directories(
        dos32_acceptance_reservation_${convention} PRIVATE
        "${OPENDOORS_ROOT}/tests/acceptance")
    opendoors_configure_dos32_target(
        dos32_acceptance_reservation_${convention} ${reservation_switch})
    target_link_libraries(
        dos32_acceptance_reservation_${convention} PRIVATE
        ${reservation_library})
endforeach()

foreach(convention IN ITEMS r s)
    if(convention STREQUAL "r")
        set(serial_library opendoors_dos32r)
        set(serial_switch 3r)
    else()
        set(serial_library opendoors_dos32s)
        set(serial_switch 3s)
    endif()

    add_executable(dos32_acceptance_uart_${convention}
        "${OPENDOORS_ROOT}/tests/acceptance/transport_door.c")
    set_target_properties(dos32_acceptance_uart_${convention} PROPERTIES
        OUTPUT_NAME "od32ua${convention}")
    target_include_directories(dos32_acceptance_uart_${convention} PRIVATE
        "${OPENDOORS_ROOT}/tests/acceptance")
    opendoors_configure_dos32_target(
        dos32_acceptance_uart_${convention} ${serial_switch})
    target_link_libraries(dos32_acceptance_uart_${convention} PRIVATE
        ${serial_library})

    add_executable(dos32_acceptance_fossil_${convention}
        "${OPENDOORS_ROOT}/tests/acceptance/transport_door.c")
    set_target_properties(dos32_acceptance_fossil_${convention} PROPERTIES
        OUTPUT_NAME "od32fo${convention}")
    target_include_directories(dos32_acceptance_fossil_${convention} PRIVATE
        "${OPENDOORS_ROOT}/tests/acceptance")
    opendoors_configure_dos32_target(
        dos32_acceptance_fossil_${convention} ${serial_switch})
    target_compile_definitions(dos32_acceptance_fossil_${convention} PRIVATE
        OD_ACCEPTANCE_DOS_FOSSIL=1)
    target_link_libraries(dos32_acceptance_fossil_${convention} PRIVATE
        ${serial_library})
endforeach()

endif()
