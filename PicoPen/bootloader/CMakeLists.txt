add_executable(picopen_bootloader
    src/main.c
)

picopen_configure_firmware_target(picopen_bootloader
    "PicoPen Bootloader"
    "PicoPen stage-1 bootloader skeleton")
picopen_set_flash_region(picopen_bootloader 0x10000000 0x00040000)
pico_add_extra_outputs(picopen_bootloader)
