function(picopen_set_flash_region TARGET FLASH_ORIGIN FLASH_LENGTH)
    set(SDK_LINKER_SCRIPT
        "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2350/memmap_default.ld")
    if(NOT EXISTS "${SDK_LINKER_SCRIPT}")
        message(FATAL_ERROR "RP2350 linker script not found: ${SDK_LINKER_SCRIPT}")
    endif()

    file(READ "${SDK_LINKER_SCRIPT}" LINKER_SCRIPT_CONTENTS)
    set(FLASH_REGION
        "FLASH(rx) : ORIGIN = ${FLASH_ORIGIN}, LENGTH = ${FLASH_LENGTH}")
    string(REPLACE "INCLUDE \"pico_flash_region.ld\"" "${FLASH_REGION}"
        LINKER_SCRIPT_CONTENTS "${LINKER_SCRIPT_CONTENTS}")

    set(GENERATED_SCRIPT
        "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_memmap.ld")
    file(WRITE "${GENERATED_SCRIPT}" "${LINKER_SCRIPT_CONTENTS}")
    pico_set_linker_script(${TARGET} "${GENERATED_SCRIPT}")
endfunction()

function(picopen_configure_firmware_target TARGET DISPLAY_NAME DESCRIPTION)
    target_include_directories(${TARGET} PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
    )
    target_compile_definitions(${TARGET} PRIVATE
        PICOPEN_VERSION="${PROJECT_VERSION}"
        PICOPEN_BUILD_TYPE="$<CONFIG>"
    )
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra)
    target_link_libraries(${TARGET} PRIVATE pico_stdlib hardware_watchdog)

    pico_enable_stdio_usb(${TARGET} 1)
    pico_enable_stdio_uart(${TARGET} 0)
    pico_set_program_name(${TARGET} "${DISPLAY_NAME}")
    pico_set_program_version(${TARGET} "${PROJECT_VERSION}")
    pico_set_program_description(${TARGET} "${DESCRIPTION}")
endfunction()

function(picopen_add_relocated_outputs TARGET FLASH_ORIGIN)
    pico_add_dis_output(${TARGET})
    pico_add_hex_output(${TARGET})
    pico_add_bin_output(${TARGET})
    pico_add_map_output(${TARGET})
    target_compile_definitions(${TARGET} PRIVATE PICO_TARGET_NAME="${TARGET}")

    pico_init_picotool()
    if(NOT picotool_FOUND)
        message(FATAL_ERROR "picotool is required to package ${TARGET}")
    endif()

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND picotool uf2 convert --quiet
            "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin" -t bin
            "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.uf2" -t uf2
            --offset ${FLASH_ORIGIN}
            --family rp2350-arm-s
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.uf2"
        VERBATIM
    )
endfunction()
