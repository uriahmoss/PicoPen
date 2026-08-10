add_executable(picopen_os
    ../kernel/capability.c
    ../kernel/ipc.c
    ../kernel/work_queue.c
    ../drivers/display/display.c
    ../drivers/keyboard/keyboard.c
    ../drivers/sd/sd.c
    ../drivers/terminal/terminal.c
    ../services/storage/fatfs_diskio.c
    ../services/storage/storage.c
    ../services/shell/shell.c
    ../services/gui/gui.c
    ../services/appearance/skin.c
    ../services/appearance/crayon_renderer.c
    ../services/appearance/synthwave_renderer.c
    ../services/audit/audit.c
    ../services/device/device.c
    ../services/workbench/workbench.c
    ../services/network/wifi.c
    ../services/settings/internal_fs.c
    ../services/settings/preferences.c
    ../services/vault/wifi_vault.c
    ../services/recovery/recovery.c
    ../services/policy/engagement.c
    ../third_party/fatfs/ff.c
    ../third_party/littlefs/lfs.c
    ../third_party/littlefs/lfs_util.c
    src/main.c
)

picopen_configure_firmware_target(picopen_os
    "PicoPen OS"
    "PicoPen minimal OS image skeleton")
picopen_set_flash_region(picopen_os 0x10051000 0x001BF000)
target_link_libraries(picopen_os PRIVATE
    hardware_spi
    hardware_i2c
    hardware_resets
    pico_cyw43_arch_lwip_poll
    pico_mbedtls
    picopen_boot_metadata
)
target_include_directories(picopen_os PRIVATE
    ${PROJECT_SOURCE_DIR}/boards/picocalc_pico2w/include
    ${PROJECT_SOURCE_DIR}/config
    ${PROJECT_SOURCE_DIR}/kernel/include
    ${PROJECT_SOURCE_DIR}/drivers/display/include
    ${PROJECT_SOURCE_DIR}/drivers/keyboard/include
    ${PROJECT_SOURCE_DIR}/drivers/sd/include
    ${PROJECT_SOURCE_DIR}/drivers/terminal/include
    ${PROJECT_SOURCE_DIR}/services/storage/include
    ${PROJECT_SOURCE_DIR}/services/shell/include
    ${PROJECT_SOURCE_DIR}/services/gui/include
    ${PROJECT_SOURCE_DIR}/services/appearance/include
    ${PROJECT_SOURCE_DIR}/services/audit/include
    ${PROJECT_SOURCE_DIR}/services/device/include
    ${PROJECT_SOURCE_DIR}/services/workbench/include
    ${PROJECT_SOURCE_DIR}/services/network/include
    ${PROJECT_SOURCE_DIR}/services/settings/include
    ${PROJECT_SOURCE_DIR}/services/vault/include
    ${PROJECT_SOURCE_DIR}/services/recovery/include
    ${PROJECT_SOURCE_DIR}/services/policy/include
    ${PROJECT_SOURCE_DIR}/third_party/fatfs
    ${PROJECT_SOURCE_DIR}/third_party/littlefs
)
target_compile_definitions(picopen_os PRIVATE
    PICO_CRT0_INCLUDE_PICOBIN_ENTRY_POINT_ITEM=1
    PICO_CRT0_INCLUDE_PICOBIN_VECTOR_TABLE_ITEM=1
    PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1
)
picopen_add_relocated_outputs(picopen_os 0x10051000)

set(PICOPEN_OS_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/picopen_os.pimg")
set(PICOPEN_OS_SLOT_UF2 "${CMAKE_CURRENT_BINARY_DIR}/picopen_os_slot.uf2")
add_custom_command(
    OUTPUT "${PICOPEN_OS_IMAGE}" "${PICOPEN_OS_SLOT_UF2}"
    COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/package_image.py"
        --bin "${CMAKE_CURRENT_BINARY_DIR}/picopen_os.bin"
        --elf "$<TARGET_FILE:picopen_os>"
        --output "${PICOPEN_OS_IMAGE}"
        --version "${PROJECT_VERSION}"
        --build-number 1
    COMMAND picotool uf2 convert --quiet
        "${PICOPEN_OS_IMAGE}" -t bin
        "${PICOPEN_OS_SLOT_UF2}" -t uf2
        --offset 0x10050000
        --family rp2350-arm-s
        --abs-block
    DEPENDS picopen_os "${PROJECT_SOURCE_DIR}/tools/package_image.py"
    VERBATIM
)
add_custom_target(picopen_os_slot ALL DEPENDS "${PICOPEN_OS_SLOT_UF2}")
