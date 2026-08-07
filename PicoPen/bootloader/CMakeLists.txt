add_executable(picopen_bootloader
    src/handoff.c
    src/image_validator.c
    src/main.c
)

picopen_configure_firmware_target(picopen_bootloader
    "PicoPen Bootloader"
    "PicoPen stage-1 bootloader skeleton")
picopen_set_flash_region(picopen_bootloader 0x10000000 0x00040000)
target_include_directories(picopen_bootloader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(picopen_bootloader PRIVATE pico_sha256)
pico_add_extra_outputs(picopen_bootloader)
