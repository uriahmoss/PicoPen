# Minimal OS image

The `picopen_os` target is independently linked at `0x10051000`, the executable
payload start of the primary image slot. It prints its identity and heartbeat,
but is not entered by the bootloader until the validation and handoff slices are
implemented.

Its UF2 is a raw relocated slot payload, not a standalone RP2350 ROM-bootable
image. It must not be flashed by itself at this stage. Slice 1C will wrap the
payload in the PicoPen manifest defined by the boot contract.
