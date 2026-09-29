function(opendoors_add_example name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE ${OPENDOORS_DEFAULT_LINK_TARGET})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/data/door.cfg"
            "$<TARGET_FILE_DIR:${name}>/door.cfg"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/data/DORINFO1.DEF"
            "$<TARGET_FILE_DIR:${name}>/DORINFO1.DEF")
endfunction()

opendoors_add_example(ex_chat ex_chat.c)
opendoors_add_example(ex_diag ex_diag.c)
opendoors_add_example(ex_hello ex_hello.c)
opendoors_add_example(ex_music ex_music.c)
opendoors_add_example(ex_ski ex_ski.c)
opendoors_add_example(ex_vote ex_vote.c ex_votio.c)
target_compile_definitions(ex_vote PRIVATE MULTINODE_AWARE)

if(WIN32)
    set_target_properties(ex_chat ex_hello ex_music ex_ski ex_vote PROPERTIES
        WIN32_EXECUTABLE TRUE)
endif()
