# Getting Started from a Fresh Pico 2 W

This procedure assumes the Pico 2 W has never been flashed. The RP2350 contains
a factory ROM bootloader, so an empty or broken application cannot permanently
remove the BOOTSEL recovery path.

## 1. Do not connect attachments yet

For the first boot, leave all external GPIO, radio, NFC, IR, and USB-host
attachments disconnected. Install the Pico 2 W in the PicoCalc only after its
orientation and header alignment have been checked.

For the easiest first flash, the Pico 2 W may be tested outside the PicoCalc.
This avoids ambiguity between the Pico 2 W micro-USB connector and the
PicoCalc's USB-C/UART routing.

## 2. Install the development tools

Required:

- Git
- Python 3
- CMake
- Ninja
- Arm GNU embedded toolchain
- Raspberry Pi Pico SDK 2.2.0, including its submodules
- Raspberry Pi prebuilt picotool 2.2.0-a4 package at
  `.deps/picotool-prebuilt/picotool`
- Raspberry Pi prebuilt pioasm package at `.deps/pico-sdk-tools/pioasm`

On Windows, the official Raspberry Pi Pico VS Code extension is the simplest
supported way to provision the SDK and toolchain. A command-line-only setup is
also valid, but `arm-none-eabi-gcc`, Ninja, and the SDK must be discoverable by
CMake.

Set `PICO_SDK_PATH` to the root of the SDK checkout. In PowerShell for the
current terminal:

```powershell
$env:PICO_SDK_PATH = "C:\path\to\pico-sdk"
```

If the SDK is cloned at `.deps/pico-sdk` inside this repository, PicoPen detects
it automatically. This also allows CMake started by VS Code to work without
inheriting `PICO_SDK_PATH` from a terminal.

The project is pinned to SDK 2.2.0 in `tools/dependencies.lock`.

## 3. Configure and build

From the `PicoPen` directory:

```powershell
cmake --preset pico2w-debug
cmake --build --preset pico2w-debug
```

The initial artifact is:

```text
build/pico2w-debug/bringup/picopen_bringup.uf2
```

## 4. Enter factory BOOTSEL mode

1. Disconnect power from the Pico 2 W.
2. Hold the BOOTSEL button on the Pico 2 W.
3. While holding BOOTSEL, connect the Pico 2 W micro-USB data cable to the PC.
4. Release BOOTSEL after the `RP2350` mass-storage volume appears.

Copy `picopen_bringup.uf2` onto that volume. The volume will disconnect as the
board resets. This is expected.

If the Pico 2 W is already installed in the closed PicoCalc and its BOOTSEL
button is inaccessible, do not force the case or guess at alternate wiring.
Open the PicoCalc using its assembly instructions or use SWD with the power off.

## 5. Verify first boot

The first image intentionally touches no PicoCalc peripheral GPIO. It:

- initializes USB serial output
- reports whether the preceding reset was caused by the watchdog
- initializes the Pico 2 W CYW43 device
- exposes explicit onboard LED control through the recovery console
- prints a heartbeat every five seconds
- accepts bounded recovery-console commands over USB serial

Open the USB serial port at any conventional baud setting; USB CDC does not use
the configured baud electrically. The expected banner begins:

```text
PicoPen safe bring-up
version: 0.0.1
target: Raspberry Pi Pico 2 W
policy: no PicoCalc GPIO or external transmitter enabled
```

Type `help` for the initial commands. The `bootsel` command returns to the ROM
USB loader without requiring physical access to the BOOTSEL button.

## 6. Recovery

If the image does not start, repeat the BOOTSEL procedure and flash a known-good
UF2. This initial program is not yet the PicoPen stage-1 bootloader and does not
alter OTP, debug permissions, secure-boot configuration, or recovery policy.

Do not program OTP or disable debugging during development. Those operations
can be irreversible.

## Next step

The stage-1 bootloader and manifested OS artifacts now build independently, but
the bootloader deliberately does not enter the OS until Slice 1D. Continue using
`picopen_bringup.uf2` on hardware for now. Do not flash `picopen_bootloader.uf2`,
`picopen_os.uf2`, or `picopen_os_slot.uf2` expecting a working boot chain.

The next implementation step is the reviewed RP2350 handoff from a validated
image. Display, keyboard, SD, and PSRAM drivers follow only after the boot chain
and board pin map are verified.
