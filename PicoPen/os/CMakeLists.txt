add_executable(picopen_os
    src/main.c
)

picopen_configure_firmware_target(picopen_os
    "PicoPen OS"
    "PicoPen minimal OS image skeleton")
picopen_set_flash_region(picopen_os 0x10051000 0x001BF000)
picopen_add_relocated_outputs(picopen_os 0x10051000)
