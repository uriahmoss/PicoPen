# Minimal OS image

The `picopen_os` target is independently linked at `0x10051000`, the executable
payload start of the primary image slot. It prints its identity and heartbeat,
but is not entered by the bootloader until the validation and handoff slices are
implemented.

`picopen_os.uf2` is the raw relocated payload. `picopen_os.pimg` contains the
4 KiB PicoPen manifest followed by that payload, and `picopen_os_slot.uf2`
places the complete image at the primary slot base. None is a standalone RP2350
ROM-bootable image. Do not flash any OS artifact by itself at this stage.
