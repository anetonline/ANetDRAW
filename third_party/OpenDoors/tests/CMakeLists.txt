function(opendoors_add_library_test name)
    add_executable("opendoors_${name}_test" "${name}_test.c")
    target_link_libraries("opendoors_${name}_test" PRIVATE OpenDoors::Static)
    set_target_properties("opendoors_${name}_test" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME "${name}" COMMAND "opendoors_${name}_test")
endfunction()

add_executable(opendoors_vote_io_test vote_io_test.c
    "${PROJECT_SOURCE_DIR}/examples/ex_votio.c")
target_include_directories(opendoors_vote_io_test PRIVATE
    "${PROJECT_SOURCE_DIR}/examples" "${PROJECT_SOURCE_DIR}/include")
set_target_properties(opendoors_vote_io_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME vote_io COMMAND opendoors_vote_io_test)

if(OPENDOORS_BUILD_STATIC)
    foreach(name IN ITEMS
        safety command_line_values exitinfo_traffic draw_box_bounds
        window_title get_key_poll input_str_range edit_rectangle edit_format
        display_path ra_substitution virtual_screen multiline_format
        multiline_growth multiline_bounds popup_bounds printf_color list_line
        dir_attributes log_open text_drop_writer list_files)
        opendoors_add_library_test(${name})
    endforeach()
    set_tests_properties(get_key_poll PROPERTIES TIMEOUT 5)

    if(UNIX)
        foreach(name IN ITEMS
            config_callback config_paths config_directory_failure
            custom_drop_lines default_config sfdoors_login_time dir_basename
            dir_stat dir_file_type unix_spawn)
            opendoors_add_library_test(${name})
        endforeach()
        set_tests_properties(
            config_callback config_paths config_directory_failure
            custom_drop_lines default_config sfdoors_login_time unix_spawn
            PROPERTIES TIMEOUT 10)
    endif()

    if(WIN32)
        opendoors_add_library_test(windows_dir)
        set_tests_properties(windows_dir PROPERTIES TIMEOUT 10)
        opendoors_add_library_test(windows_chat_failure)
        set_target_properties(opendoors_windows_chat_failure_test PROPERTIES
            WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(NOT WIN32)
    add_executable(opendoors_fallback_format_test
        format_fallback_test.c "${PROJECT_SOURCE_DIR}/src/ODFmtFB.c")
    target_include_directories(opendoors_fallback_format_test PRIVATE
        "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}")
    target_compile_options(opendoors_fallback_format_test PRIVATE
        -std=c90 -pedantic-errors -Werror)
    target_link_libraries(opendoors_fallback_format_test PRIVATE m)
    add_test(NAME fallback_format COMMAND opendoors_fallback_format_test)
endif()

if(OPENDOORS_BUILD_SHARED)
    add_executable(opendoors_shared_smoke link_smoke.c)
    target_link_libraries(opendoors_shared_smoke PRIVATE OpenDoors::Shared)
    set_target_properties(opendoors_shared_smoke PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME shared_link_smoke COMMAND opendoors_shared_smoke)
endif()
if(OPENDOORS_BUILD_STATIC)
    add_executable(opendoors_static_smoke link_smoke.c)
    target_link_libraries(opendoors_static_smoke PRIVATE OpenDoors::Static)
    set_target_properties(opendoors_static_smoke PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME static_link_smoke COMMAND opendoors_static_smoke)
endif()

set(OPENDOORS_ACCEPTANCE_LIBRARY "${OPENDOORS_DEFAULT_LINK_TARGET}")
add_subdirectory(acceptance)
