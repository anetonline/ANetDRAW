cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(OpenDoorsAcceptance LANGUAGES C CXX)
    include(CTest)
    set(OPENDOORS_VARIANT "Shared" CACHE STRING
        "Installed OpenDoors target to test")
    find_package(OpenDoors 6.3 CONFIG REQUIRED COMPONENTS ${OPENDOORS_VARIANT})
    set(OPENDOORS_ACCEPTANCE_LIBRARY "OpenDoors::${OPENDOORS_VARIANT}")
elseif(NOT DEFINED OPENDOORS_ACCEPTANCE_LIBRARY)
    if(TARGET OpenDoors::Static)
        set(OPENDOORS_ACCEPTANCE_LIBRARY OpenDoors::Static)
    else()
        set(OPENDOORS_ACCEPTANCE_LIBRARY OpenDoors::Shared)
    endif()
endif()

option(OPENDOORS_ACCEPTANCE_EXTENDED
    "Run transport-driven interactive acceptance scenarios" OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(opendoors_acceptance_library_path name)
    if(WIN32)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
            set_tests_properties("${name}" PROPERTIES
                ENVIRONMENT_MODIFICATION
                    "PATH=path_list_prepend:$<TARGET_FILE_DIR:${OPENDOORS_ACCEPTANCE_LIBRARY}>")
        else()
            set_tests_properties("${name}" PROPERTIES
                ENVIRONMENT
                    "PATH=$<TARGET_FILE_DIR:${OPENDOORS_ACCEPTANCE_LIBRARY}>")
        endif()
    endif()
endfunction()

function(opendoors_acceptance_test name source)
    add_executable("opendoors_acceptance_${name}" "${source}")
    target_link_libraries("opendoors_acceptance_${name}"
        PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")
    target_include_directories("opendoors_acceptance_${name}"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties("opendoors_acceptance_${name}" PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED TRUE
        C_EXTENSIONS TRUE)
    add_test(NAME "acceptance.${name}"
        COMMAND "opendoors_acceptance_${name}" ${ARGN})
    set_tests_properties("acceptance.${name}" PROPERTIES TIMEOUT 20)
    opendoors_acceptance_library_path("acceptance.${name}")
endfunction()

opendoors_acceptance_test(public_surface public_surface_test.c)
opendoors_acceptance_test(lifecycle lifecycle_test.c)
opendoors_acceptance_test(lifecycle_init_exit lifecycle_init_exit_test.c)
opendoors_acceptance_test(command_line command_line_test.c)
opendoors_acceptance_test(screen screen_test.c)
if(WIN32)
    target_link_libraries(opendoors_acceptance_screen PRIVATE ws2_32)
    opendoors_acceptance_test(windows_screen windows_screen_test.c)
    set_target_properties(opendoors_acceptance_windows_screen PROPERTIES
        WIN32_EXECUTABLE TRUE)
    target_link_libraries(opendoors_acceptance_windows_screen PRIVATE ws2_32)
    opendoors_acceptance_test(windows_thread_handoff
        windows_thread_handoff_test.c)
    set_target_properties(opendoors_acceptance_windows_thread_handoff
        PROPERTIES WIN32_EXECUTABLE TRUE)

    add_executable(opendoors_acceptance_windows_gui_parser_mismatch
        windows_gui_parser_mismatch_test.c)
    target_link_libraries(opendoors_acceptance_windows_gui_parser_mismatch
        PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")
    set(_opendoors_windows_launcher)
    foreach(_opendoors_launcher_arg IN LISTS CMAKE_CROSSCOMPILING_EMULATOR)
        list(APPEND _opendoors_windows_launcher
            --launcher "${_opendoors_launcher_arg}")
    endforeach()
    add_test(NAME acceptance.windows_gui_parser_mismatch
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.py"
            ${_opendoors_windows_launcher}
            --expected "od_parse_cmd_line() requires a Windows GUI executable"
            "$<TARGET_FILE:opendoors_acceptance_windows_gui_parser_mismatch>")
    set_tests_properties(acceptance.windows_gui_parser_mismatch PROPERTIES
        TIMEOUT 20)
    opendoors_acceptance_library_path(
        acceptance.windows_gui_parser_mismatch)

    add_executable(opendoors_acceptance_windows_console_parser_mismatch
        WIN32 windows_console_parser_mismatch_test.c)
    target_link_libraries(opendoors_acceptance_windows_console_parser_mismatch
        PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")
    add_test(NAME acceptance.windows_console_parser_mismatch
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.py"
            ${_opendoors_windows_launcher}
            --expected "od_parse_cmd_line_cons() requires a Windows console executable"
            "$<TARGET_FILE:opendoors_acceptance_windows_console_parser_mismatch>")
    set_tests_properties(acceptance.windows_console_parser_mismatch PROPERTIES
        TIMEOUT 20)
    opendoors_acceptance_library_path(
        acceptance.windows_console_parser_mismatch)

    set(_opendoors_static_console_library OpenDoors::StaticConsole)
    if(OPENDOORS_VARIANT STREQUAL "StaticMT")
        set(_opendoors_static_console_library OpenDoors::StaticMTConsole)
    endif()
    if(TARGET "${_opendoors_static_console_library}")
        add_executable(opendoors_acceptance_windows_console
            windows_console_test.c)
        target_link_libraries(opendoors_acceptance_windows_console
            PRIVATE "${_opendoors_static_console_library}")
        target_include_directories(opendoors_acceptance_windows_console
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        set_target_properties(opendoors_acceptance_windows_console PROPERTIES
            C_STANDARD 90
            C_STANDARD_REQUIRED TRUE
            C_EXTENSIONS TRUE)
        add_test(NAME acceptance.windows_console
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/expect_success.py"
                ${_opendoors_windows_launcher}
                --marker "OPENDOORS_WINDOWS_CONSOLE_ACCEPTANCE_OK"
                "$<TARGET_FILE:opendoors_acceptance_windows_console>")
        set_tests_properties(acceptance.windows_console PROPERTIES TIMEOUT 20)
    endif()

    if(TARGET OpenDoors::SharedConsole)
        add_executable(opendoors_acceptance_windows_console_shared
            windows_console_test.c)
        target_link_libraries(opendoors_acceptance_windows_console_shared
            PRIVATE OpenDoors::SharedConsole)
        target_include_directories(opendoors_acceptance_windows_console_shared
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        set_target_properties(opendoors_acceptance_windows_console_shared
            PROPERTIES
                C_STANDARD 90
                C_STANDARD_REQUIRED TRUE
                C_EXTENSIONS TRUE)
        add_test(NAME acceptance.windows_console_shared
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/expect_success.py"
                ${_opendoors_windows_launcher}
                --marker "OPENDOORS_WINDOWS_CONSOLE_ACCEPTANCE_OK"
                "$<TARGET_FILE:opendoors_acceptance_windows_console_shared>")
        set_tests_properties(acceptance.windows_console_shared PROPERTIES
            TIMEOUT 20)
        opendoors_acceptance_library_path(
            acceptance.windows_console_shared)
    endif()
endif()
opendoors_acceptance_test(components components_test.c)
opendoors_acceptance_test(custom_drop_config custom_drop_config_test.c)
opendoors_acceptance_test(logging logging_test.c)
opendoors_acceptance_test(reservation reservation_test.c)

add_executable(opendoors_acceptance_dropfiles dropfiles_test.c)
target_link_libraries(opendoors_acceptance_dropfiles
    PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")
target_include_directories(opendoors_acceptance_dropfiles
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(opendoors_acceptance_dropfiles PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS TRUE)
foreach(format IN ITEMS dorinfo chain sfdoors doorway gap wildcat callinfo
        tribbs door32 custom exitinfo qbbs275 ra1 ra2)
    add_test(NAME "acceptance.dropfiles.${format}"
        COMMAND opendoors_acceptance_dropfiles "${format}")
    set_tests_properties("acceptance.dropfiles.${format}" PROPERTIES
        TIMEOUT 20 RESOURCE_LOCK opendoors-dropfiles)
    opendoors_acceptance_library_path("acceptance.dropfiles.${format}")
endforeach()

add_executable(opendoors_acceptance_spawn_child spawn_child.c)
opendoors_acceptance_test(spawn spawn_test.c
    "$<TARGET_FILE:opendoors_acceptance_spawn_child>")

if(NOT CMAKE_CXX_COMPILER_LOADED)
    enable_language(CXX)
endif()
add_library(opendoors_acceptance_cpp_compile OBJECT cpp_surface_test.cpp)
target_link_libraries(opendoors_acceptance_cpp_compile
    PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")

add_test(NAME acceptance.contract_manifest
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/check_public_contract.py")
set_tests_properties(acceptance.contract_manifest PROPERTIES
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
add_test(NAME acceptance.symbols
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/check_public_symbols.py"
        "$<TARGET_FILE:${OPENDOORS_ACCEPTANCE_LIBRARY}>"
        --linker "${CMAKE_LINKER}"
        --platform "$<IF:$<BOOL:${WIN32}>,windows,unix>")

if(OPENDOORS_ACCEPTANCE_EXTENDED)
    add_executable(opendoors_acceptance_transport_door transport_door.c)
    target_link_libraries(opendoors_acceptance_transport_door
        PRIVATE "${OPENDOORS_ACCEPTANCE_LIBRARY}")
    if(WIN32)
        target_link_libraries(opendoors_acceptance_transport_door
            PRIVATE ws2_32)
    endif()
    target_include_directories(opendoors_acceptance_transport_door
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(opendoors_acceptance_transport_door PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED TRUE
        C_EXTENSIONS TRUE)
    set(_opendoors_socket_launcher)
    foreach(_opendoors_launcher_arg IN LISTS CMAKE_CROSSCOMPILING_EMULATOR)
        list(APPEND _opendoors_socket_launcher
            --launcher "${_opendoors_launcher_arg}")
    endforeach()
    if(WIN32)
        list(APPEND _opendoors_socket_launcher --tcp)
    endif()
    foreach(_opendoors_scenario IN ITEMS
        input interactive edit display emulation listing popup session)
        add_test(NAME "acceptance.transport.${_opendoors_scenario}"
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/socket_controller.py"
                ${_opendoors_socket_launcher}
                "${_opendoors_scenario}"
                "$<TARGET_FILE:opendoors_acceptance_transport_door>")
        set_tests_properties(
            "acceptance.transport.${_opendoors_scenario}" PROPERTIES
            TIMEOUT 40 LABELS "acceptance-extended")
        opendoors_acceptance_library_path(
            "acceptance.transport.${_opendoors_scenario}")
    endforeach()
endif()
