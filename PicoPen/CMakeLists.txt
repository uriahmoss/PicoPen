cmake_minimum_required(VERSION 3.13...3.29)

# The first supported target is deliberately fixed. Supporting another board
# requires a reviewed board definition rather than an accidental CMake override.
set(PICO_BOARD pico2_w CACHE STRING "PicoPen target board")

# Git for Windows exposes both a cmd shim and a native bin entry. FetchContent
# submodule updates require Git's bundled shell utilities, which the bin entry
# locates reliably when CMake is launched from PowerShell.
if(WIN32 AND EXISTS "C:/Program Files/Git/bin/git.exe")
    set(GIT_EXECUTABLE "C:/Program Files/Git/bin/git.exe"
        CACHE FILEPATH "Git executable used by CMake" FORCE)
endif()

if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()

# A repository-local SDK is the documented manual-install fallback and should
# also work when VS Code starts CMake without inheriting a terminal environment.
if(NOT PICO_SDK_PATH AND
   EXISTS "${CMAKE_CURRENT_LIST_DIR}/.deps/pico-sdk/external/pico_sdk_import.cmake")
    set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/.deps/pico-sdk")
    message(STATUS "Using repository-local Pico SDK: ${PICO_SDK_PATH}")
endif()

if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR
        "Pico SDK 2.2.0 was not found. Set PICO_SDK_PATH or clone it into "
        ".deps/pico-sdk. See docs/GETTING_STARTED.md.")
endif()

# RP2350 artifact generation requires picotool. Prefer Raspberry Pi's official
# prebuilt package so Windows hosts do not need a second native C++ toolchain.
if(EXISTS
   "${CMAKE_CURRENT_LIST_DIR}/.deps/picotool-prebuilt/picotool/picotoolConfig.cmake")
    set(picotool_DIR
        "${CMAKE_CURRENT_LIST_DIR}/.deps/picotool-prebuilt/picotool"
        CACHE PATH "Official prebuilt picotool package" FORCE)
endif()

if(EXISTS
   "${CMAKE_CURRENT_LIST_DIR}/.deps/pico-sdk-tools/pioasm/pioasmConfig.cmake")
    set(pioasm_DIR
        "${CMAKE_CURRENT_LIST_DIR}/.deps/pico-sdk-tools/pioasm"
        CACHE PATH "Official prebuilt pioasm package" FORCE)
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picopen VERSION 0.0.1 LANGUAGES C CXX ASM)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

pico_sdk_init()

include(cmake/PicoPenTarget.cmake)

add_library(picopen_boot_metadata STATIC src/boot_metadata.c)
target_include_directories(picopen_boot_metadata PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(picopen_boot_metadata PUBLIC hardware_flash)

add_subdirectory(bootloader)
add_subdirectory(bringup)
add_subdirectory(os)
