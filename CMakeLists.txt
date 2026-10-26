cmake_minimum_required(VERSION 3.16)
project(atm_qt LANGUAGES CXX)

string(REGEX MATCH "[^ -~]" _non_ascii_source_path "${CMAKE_SOURCE_DIR}")
if(_non_ascii_source_path)
    message(WARNING "Source path contains non-ASCII characters; moc may fail. Move the project to an ASCII-only path: ${CMAKE_SOURCE_DIR}")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ENABLE_TSAN "ThreadSanitizer for messaging_core/tests" OFF)
option(ENABLE_ASAN "AddressSanitizer for messaging_core/tests" OFF)
option(ENABLE_GTEST_FETCH "Download GoogleTest when it is not installed" OFF)

if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

add_library(messaging_core STATIC
    libs/messaging/queue.cpp
    libs/messaging/dispatcher.cpp
    core/actors/atm_actor.cpp
    core/actors/bank_actor.cpp
    core/bank_database.cpp
    core/actors/ui_actor.cpp
)
target_include_directories(messaging_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
find_package(Qt6 COMPONENTS Core Sql QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Sql REQUIRED)
    set(ATM_QT_SQL_LIB Qt5::Core Qt5::Sql)
else()
    set(ATM_QT_SQL_LIB Qt6::Core Qt6::Sql)
endif()
target_link_libraries(messaging_core PUBLIC Threads::Threads ${ATM_QT_SQL_LIB})

enable_testing()
find_package(GTest QUIET)
if(NOT GTest_FOUND AND ENABLE_GTEST_FETCH)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    FetchContent_MakeAvailable(googletest)
endif()
if(GTest_FOUND OR ENABLE_GTEST_FETCH)
    add_executable(messaging_tests tests/messaging_tests.cpp)
    target_link_libraries(messaging_tests PRIVATE messaging_core GTest::gtest_main Threads::Threads)
    add_test(NAME messaging_tests COMMAND messaging_tests)
else()
    message(STATUS "GoogleTest not found; messaging_tests is disabled. Set ENABLE_GTEST_FETCH=ON to download it.")
endif()

if(ENABLE_TSAN)
    if(MSVC)
        message(FATAL_ERROR "MSVC 不支持 TSan，请用 WSL2/Linux + GCC/Clang")
    endif()
    target_compile_options(messaging_core PRIVATE -fsanitize=thread -g -O1)
    target_link_options(messaging_core PRIVATE -fsanitize=thread)
    if(TARGET messaging_tests)
        target_compile_options(messaging_tests PRIVATE -fsanitize=thread -g -O1)
        target_link_options(messaging_tests PRIVATE -fsanitize=thread)
    endif()
endif()
if(ENABLE_ASAN)
    if(MSVC)
        target_compile_options(messaging_core PRIVATE /fsanitize=address)
        if(TARGET messaging_tests)
            target_compile_options(messaging_tests PRIVATE /fsanitize=address)
        endif()
        target_compile_definitions(messaging_core PRIVATE _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
        if(TARGET messaging_tests)
            target_compile_definitions(messaging_tests PRIVATE _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
        endif()
        foreach(target gtest gtest_main gmock gmock_main)
            if(TARGET ${target})
                target_compile_definitions(${target} PRIVATE _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
            endif()
        endforeach()
    else()
        target_compile_options(messaging_core PRIVATE -fsanitize=address -g -O1)
        target_link_options(messaging_core PRIVATE -fsanitize=address)
        if(TARGET messaging_tests)
            target_compile_options(messaging_tests PRIVATE -fsanitize=address -g -O1)
            target_link_options(messaging_tests PRIVATE -fsanitize=address)
        endif()
    endif()
endif()

add_executable(atm_cli app/cli_main.cpp)
target_link_libraries(atm_cli PRIVATE messaging_core)
if(ENABLE_TSAN AND NOT MSVC)
    target_compile_options(atm_cli PRIVATE -fsanitize=thread -g -O1)
    target_link_options(atm_cli PRIVATE -fsanitize=thread)
endif()
if(ENABLE_ASAN)
    if(MSVC)
        target_compile_options(atm_cli PRIVATE /fsanitize=address)
        target_compile_definitions(atm_cli PRIVATE _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
    else()
        target_compile_options(atm_cli PRIVATE -fsanitize=address -g -O1)
        target_link_options(atm_cli PRIVATE -fsanitize=address)
    endif()
endif()

if(ENABLE_ASAN AND MSVC AND TARGET messaging_tests)
    add_custom_command(TARGET messaging_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/clang_rt.asan_dynamic-x86_64.dll"
        "$<TARGET_FILE_DIR:messaging_tests>")
endif()

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Widgets REQUIRED)
    set(ATM_QT_LIB Qt5::Widgets)
else()
    set(ATM_QT_LIB Qt6::Widgets)
endif()
set(CMAKE_AUTOMOC ON)
file(TO_CMAKE_PATH "$ENV{TEMP}" ATM_QT_TEMP_DIR)
set(ATM_QT_MOC_INPUT_DIR "${ATM_QT_TEMP_DIR}/atm_qt_moc_input")
file(MAKE_DIRECTORY "${ATM_QT_MOC_INPUT_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/app/qt_display_bridge.hpp"
    "${ATM_QT_MOC_INPUT_DIR}/qt_display_bridge.hpp"
    COPYONLY
)
if(Qt6_FOUND)
    set(ATM_QT_CORE_INCLUDE_DIRS ${Qt6Core_INCLUDE_DIRS})
    get_filename_component(ATM_QT_BIN_DIR "${Qt6Core_DIR}/../../../bin" ABSOLUTE)
else()
    set(ATM_QT_CORE_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS})
    get_filename_component(ATM_QT_BIN_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
endif()
find_program(ATM_QT_MOC_EXECUTABLE
    NAMES moc moc.exe
    HINTS "${ATM_QT_BIN_DIR}")
if(NOT ATM_QT_MOC_EXECUTABLE)
    message(FATAL_ERROR "Could not locate the moc executable for the selected Qt package")
endif()
set(ATM_QT_MOC_INCLUDE_ARGS)
foreach(ATM_QT_CORE_INCLUDE_DIR IN LISTS ATM_QT_CORE_INCLUDE_DIRS)
    list(APPEND ATM_QT_MOC_INCLUDE_ARGS "-I${ATM_QT_CORE_INCLUDE_DIR}")
endforeach()
set(ATM_QT_MOC_OUTPUT "${ATM_QT_MOC_INPUT_DIR}/atm_qt_moc_qt_display_bridge.cpp")
add_custom_command(
    OUTPUT "${ATM_QT_MOC_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/app/qt_display_bridge.hpp"
            "${ATM_QT_MOC_INPUT_DIR}/qt_display_bridge.hpp"
    COMMAND "${ATM_QT_MOC_EXECUTABLE}"
            ${ATM_QT_MOC_INCLUDE_ARGS}
            "-I${ATM_QT_MOC_INPUT_DIR}"
            -o "${ATM_QT_MOC_OUTPUT}"
            "${ATM_QT_MOC_INPUT_DIR}/qt_display_bridge.hpp"
    DEPENDS app/qt_display_bridge.hpp
    VERBATIM
)
add_executable(atm_qt
        app/main.cpp
        app/main_window.cpp
        app/admin_window.cpp
        app/qt_display_bridge.cpp
        "${ATM_QT_MOC_OUTPUT}"
)
target_link_libraries(atm_qt PRIVATE messaging_core ${ATM_QT_LIB})
set_target_properties(atm_qt PROPERTIES AUTOMOC OFF)
target_include_directories(atm_qt PRIVATE
    ${ATM_QT_MOC_INPUT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/actors
    ${CMAKE_CURRENT_SOURCE_DIR}/core/messages
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

if(WIN32)
    get_target_property(ATM_QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    if(NOT ATM_QT_QMAKE_EXECUTABLE)
        get_target_property(ATM_QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    endif()
    get_filename_component(ATM_QT_BIN_DIR "${ATM_QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(ATM_WINDEPLOYQT_EXECUTABLE NAMES windeployqt.exe HINTS "${ATM_QT_BIN_DIR}")
    if(ATM_WINDEPLOYQT_EXECUTABLE)
        foreach(ATM_DEPLOY_TARGET atm_cli atm_qt)
            add_custom_command(TARGET ${ATM_DEPLOY_TARGET} POST_BUILD
                COMMAND "${ATM_WINDEPLOYQT_EXECUTABLE}"
                        --compiler-runtime --no-translations --no-system-d3d-compiler
                        "$<TARGET_FILE:${ATM_DEPLOY_TARGET}>"
                COMMENT "Deploying Qt runtime for ${ATM_DEPLOY_TARGET}"
                VERBATIM)
        endforeach()
    else()
        message(WARNING "windeployqt not found; Windows executable targets will not be self-contained")
    endif()
endif()

