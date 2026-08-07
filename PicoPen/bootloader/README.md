# Bootloader source

The `picopen_bootloader` target is linked into the 256 KiB stage-1 region at
`0x10000000`. During Slice 1B it is deliberately only a recovery skeleton: it
does not validate or transfer control to an OS image yet.

Do not flash the bootloader UF2 expecting a working OS boot chain before Slices
1C and 1D are complete. RP2350 ROM BOOTSEL remains available for recovery.
