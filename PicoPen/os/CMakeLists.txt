add_executable(picopen_os
    src/main.c
)

picopen_configure_firmware_target(picopen_os
    "PicoPen OS"
    "PicoPen minimal OS image skeleton")
picopen_set_flash_region(picopen_os 0x10051000 0x001BF000)
picopen_add_relocated_outputs(picopen_os 0x10051000)

set(PICOPEN_OS_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/picopen_os.pimg")
set(PICOPEN_OS_SLOT_UF2 "${CMAKE_CURRENT_BINARY_DIR}/picopen_os_slot.uf2")
add_custom_command(TARGET picopen_os POST_BUILD
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
    BYPRODUCTS "${PICOPEN_OS_IMAGE}" "${PICOPEN_OS_SLOT_UF2}"
    VERBATIM
)
