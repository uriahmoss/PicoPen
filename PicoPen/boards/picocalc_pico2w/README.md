# PicoCalc V2.0 with Pico 2 W

The first pin-map draft was transcribed from ClockworkPi's official
`clockwork_Mainboard_V2.0_Schematic.pdf` and cross-checked against its
`picocalc_helloworld` reference firmware at upstream commit
`e8e38aa4b502d31a0d789911bbd84ec9eb0068b9`.

The source schematic has SHA-256
`b6fd79c6cbe9b0825525211d0f2e3e0fd269a2f9a6fc22580452de3012156ede`.
The upstream checkout is a local ignored dependency and is not vendored into
PicoPen.

`include/picopen/board_pins.h` currently records only the display and keyboard
buses. The target unit's `CPI 2.0` marking physically confirmed the revision.
Each peripheral must still be enabled only by its bounded driver; unused buses
and all external transmitters remain inactive.
