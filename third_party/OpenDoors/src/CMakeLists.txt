set(OPENDOORS_SOURCES
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
    ODStr.c
    ODSync.c
    ODUtil.c
    ODWCat.c
    ODWin.c
)
if(WIN32)
    list(APPEND OPENDOORS_SOURCES ODConsole.c ODFrame.c ODRes.rc)
endif()

set(OPENDOORS_PUBLIC_HEADERS
    "${PROJECT_SOURCE_DIR}/include/ODStat.h"
    "${PROJECT_SOURCE_DIR}/include/OpenDoor.h")
set(OPENDOORS_PRIVATE_HEADERS
    ODCom.h
    ODConsole.h
    ODCore.h
    ODFormat.h
    ODFrame.h
    ODGen.h
    ODInEx.h
    ODInQue.h
    ODKrnl.h
    ODMulti.h
    ODPlat.h
    ODRes.h
    ODRsv.h
    ODSafe.h
    ODScrn.h
    ODStr.h
    ODSwap.h
    ODSync.h
    ODTypes.h
    ODUtil.h
    ODVScrn.h)

if(NOT OPENDOORS_BUILD_SHARED AND NOT OPENDOORS_BUILD_STATIC)
    message(FATAL_ERROR
        "At least one of OPENDOORS_BUILD_SHARED or OPENDOORS_BUILD_STATIC must be enabled")
endif()
if(OPENDOORS_BUILD_MSVC_STATIC_MT)
    if(NOT MSVC)
        message(FATAL_ERROR
            "OPENDOORS_BUILD_MSVC_STATIC_MT is only supported with MSVC")
    endif()
    if(NOT OPENDOORS_BUILD_STATIC)
        message(FATAL_ERROR
            "OPENDOORS_BUILD_MSVC_STATIC_MT requires OPENDOORS_BUILD_STATIC")
    endif()
endif()

check_include_file(inttypes.h OPENDOORS_HAVE_INTTYPES_H)
check_symbol_exists(vsnprintf "stdio.h" OPENDOORS_HAVE_VSNPRINTF)

function(opendoors_configure_library target)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE "${PROJECT_SOURCE_DIR}")
    target_compile_features(${target} PUBLIC c_std_99)
    if(OPENDOORS_HAVE_INTTYPES_H)
        target_compile_definitions(${target} PRIVATE HAS_INTTYPES_H)
    endif()
    if(OPENDOORS_HAVE_VSNPRINTF)
        target_compile_definitions(${target} PRIVATE
            OPENDOORS_HAVE_VSNPRINTF=1)
    endif()
    if(OPENDOORS_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /WX)
            target_link_options(${target} PRIVATE /WX)
        else()
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
    if(APPLE)
        target_compile_definitions(${target} PRIVATE __unix__)
    endif()
    if(WIN32)
        target_link_libraries(${target} PRIVATE
            ws2_32 user32 gdi32 advapi32 shell32 uuid comctl32)
    endif()
    set_target_properties(${target} PROPERTIES
        C_VISIBILITY_PRESET hidden
        POSITION_INDEPENDENT_CODE ON)
endfunction()

set(OPENDOORS_LIBRARY_TARGETS)
set(OPENDOORS_INTERFACE_TARGETS)

if(OPENDOORS_BUILD_SHARED)
    add_library(opendoors_shared SHARED
        ${OPENDOORS_SOURCES} ${OPENDOORS_PUBLIC_HEADERS}
        ${OPENDOORS_PRIVATE_HEADERS})
    add_library(OpenDoors::Shared ALIAS opendoors_shared)
    opendoors_configure_library(opendoors_shared)
    set_target_properties(opendoors_shared PROPERTIES EXPORT_NAME Shared)
    list(APPEND OPENDOORS_LIBRARY_TARGETS opendoors_shared)
endif()
if(OPENDOORS_BUILD_STATIC)
    add_library(opendoors_static STATIC
        ${OPENDOORS_SOURCES} ${OPENDOORS_PUBLIC_HEADERS}
        ${OPENDOORS_PRIVATE_HEADERS})
    add_library(OpenDoors::Static ALIAS opendoors_static)
    opendoors_configure_library(opendoors_static)
    set_target_properties(opendoors_static PROPERTIES EXPORT_NAME Static)
    list(APPEND OPENDOORS_LIBRARY_TARGETS opendoors_static)
endif()
if(OPENDOORS_BUILD_MSVC_STATIC_MT)
    add_library(opendoors_static_mt STATIC
        ${OPENDOORS_SOURCES} ${OPENDOORS_PUBLIC_HEADERS}
        ${OPENDOORS_PRIVATE_HEADERS})
    add_library(OpenDoors::StaticMT ALIAS opendoors_static_mt)
    opendoors_configure_library(opendoors_static_mt)
    set_target_properties(opendoors_static_mt PROPERTIES
        EXPORT_NAME StaticMT
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    list(APPEND OPENDOORS_LIBRARY_TARGETS opendoors_static_mt)
endif()

if(WIN32)
    if(OPENDOORS_BUILD_SHARED)
        add_library(opendoors_shared_console INTERFACE)
        add_library(OpenDoors::SharedConsole ALIAS opendoors_shared_console)
        target_link_libraries(opendoors_shared_console INTERFACE opendoors_shared)
        target_compile_definitions(opendoors_shared_console
            INTERFACE OD_WINDOWS_CONSOLE)
        set_target_properties(opendoors_shared_console PROPERTIES
            EXPORT_NAME SharedConsole)
        list(APPEND OPENDOORS_INTERFACE_TARGETS opendoors_shared_console)
    endif()
    if(OPENDOORS_BUILD_STATIC)
        add_library(opendoors_static_console INTERFACE)
        add_library(OpenDoors::StaticConsole ALIAS opendoors_static_console)
        target_link_libraries(opendoors_static_console INTERFACE opendoors_static)
        target_compile_definitions(opendoors_static_console
            INTERFACE OD_WINDOWS_CONSOLE)
        set_target_properties(opendoors_static_console PROPERTIES
            EXPORT_NAME StaticConsole)
        list(APPEND OPENDOORS_INTERFACE_TARGETS opendoors_static_console)
    endif()
    if(OPENDOORS_BUILD_MSVC_STATIC_MT)
        add_library(opendoors_static_mt_console INTERFACE)
        add_library(OpenDoors::StaticMTConsole ALIAS opendoors_static_mt_console)
        target_link_libraries(opendoors_static_mt_console
            INTERFACE opendoors_static_mt)
        target_compile_definitions(opendoors_static_mt_console
            INTERFACE OD_WINDOWS_CONSOLE)
        set_target_properties(opendoors_static_mt_console PROPERTIES
            EXPORT_NAME StaticMTConsole)
        list(APPEND OPENDOORS_INTERFACE_TARGETS opendoors_static_mt_console)
    endif()
endif()

if(WIN32)
    if(OPENDOORS_BUILD_SHARED)
        set_target_properties(opendoors_shared PROPERTIES
            PREFIX ""
            RUNTIME_OUTPUT_NAME ODoors63
            ARCHIVE_OUTPUT_NAME ODoorW)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            if(MSVC)
                target_sources(opendoors_shared PRIVATE OpenDoor.def)
                target_compile_definitions(opendoors_shared PRIVATE
                    OPENDOORS_USE_DEF_EXPORTS)
            elseif(MINGW)
                target_sources(opendoors_shared PRIVATE OpenDoorMinGW.def)
            endif()
        else()
            target_sources(opendoors_shared PRIVATE OpenDoor64.def)
            target_compile_definitions(opendoors_shared PRIVATE
                OPENDOORS_USE_DEF_EXPORTS)
        endif()
        if(MINGW)
            target_link_options(opendoors_shared PRIVATE -static-libgcc)
        endif()
    endif()
    if(OPENDOORS_BUILD_STATIC)
        set_target_properties(opendoors_static PROPERTIES
            ARCHIVE_OUTPUT_NAME ODoors-static)
        target_compile_definitions(opendoors_static PUBLIC OD_WIN32_STATIC)
    endif()
    if(OPENDOORS_BUILD_MSVC_STATIC_MT)
        set_target_properties(opendoors_static_mt PROPERTIES
            ARCHIVE_OUTPUT_NAME ODoors-static-mt)
        target_compile_definitions(opendoors_static_mt PUBLIC OD_WIN32_STATIC)
    endif()
else()
    if(OPENDOORS_BUILD_SHARED)
        set_target_properties(opendoors_shared PROPERTIES
            OUTPUT_NAME ODoors
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
    endif()
    if(OPENDOORS_BUILD_STATIC)
        set_target_properties(opendoors_static PROPERTIES OUTPUT_NAME ODoors)
    endif()
endif()

foreach(target IN LISTS OPENDOORS_LIBRARY_TARGETS)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endforeach()

install(TARGETS ${OPENDOORS_LIBRARY_TARGETS} ${OPENDOORS_INTERFACE_TARGETS}
    EXPORT OpenDoorsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${OPENDOORS_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT OpenDoorsTargets
    FILE OpenDoorsTargets.cmake
    NAMESPACE OpenDoors::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenDoors)
