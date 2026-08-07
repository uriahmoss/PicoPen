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
- deliberately leaves CYW43/Wi-Fi and its onboard LED uninitialized
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

The recovery target is linked into a hard 256 KiB region. A linker overflow is
a build failure, preventing it from overwriting boot metadata.

## 6. Recovery

If the image does not start, repeat the BOOTSEL procedure and flash a known-good
UF2. This initial program is not yet the PicoPen stage-1 bootloader and does not
alter OTP, debug permissions, secure-boot configuration, or recovery policy.

Do not program OTP or disable debugging during development. Those operations
can be irreversible.

## Next step

The stable hardware image remains `picopen_bringup.uf2`. Slice 1D now contains a
complete candidate bootloader-to-OS handoff, but it remains an acceptance-test
build until the physical transfer is confirmed using the procedure below. Do
not flash the raw `picopen_os.uf2`; it does not contain the slot manifest.

Display, keyboard, SD, and PSRAM drivers follow only after the boot chain and
board pin map are verified.

## Slice 1D hardware test

Only perform this test with physical access to the Pico 2 W BOOTSEL button and
a known-good copy of `picopen_bringup.uf2` available.

1. Build the `pico2w-debug` preset and confirm all host tests pass.
2. Enter ROM BOOTSEL and copy
   `build/pico2w-debug/os/picopen_os_slot.uf2` first.
3. The existing bring-up image should still start because this first file only
   fills the primary OS slot. Use its `bootsel` command to return to ROM BOOTSEL.
4. Copy `build/pico2w-debug/bootloader/picopen_bootloader.uf2`.
5. Open USB serial. A successful handoff prints:

```text
PicoPen minimal OS
boot-source: direct or unknown
status: minimal OS running
boot-success: confirmed; attempts reset to 0/3
```

The OS then prints a heartbeat every five seconds. If validation or ROM chaining
fails, the bootloader remains at its USB recovery console and accepts `bootsel`.
If USB recovery is unavailable, use the physical BOOTSEL button and restore
`picopen_bringup.uf2`.

The candidate handoff is guarded by a 20-second watchdog. The OS resets the USB
controller and has 15 seconds to enumerate CDC without requiring a terminal to
open the port. If it cannot, the device
automatically returns to the bootloader recovery console and reports one of
`chain-started`, `os-entered`, or `os-usb-timeout` as the last checkpoint.
If Windows opens the recovered COM port after the initial banner, type `status`
to print the checkpoint again.

Warm `bootsel` commands record a one-shot request in watchdog scratch memory and
perform a hardware reset. On the next boot, the request is cleared and ROM
BOOTSEL is entered before firmware initializes USB. This prevents a live
TinyUSB CDC instance from being handed directly to the ROM USB stack.

The bootloader deliberately does not initialize USB before a valid handoff.
This prevents the OS from inheriting an active TinyUSB peripheral instance and
ensures the chained OS can enumerate its own USB CDC port.

For the Slice 1E failure-path test, disconnect USB and then power-cycle the
PicoCalc from its battery power. The power cycle is required because an OS that
has already confirmed boot intentionally disables the boot watchdog and keeps
running after USB is removed. Leave USB disconnected during the new OS USB
wait. The watchdog retries the validated OS up to three times and then exposes
the bootloader recovery console. After at least 65 seconds, reconnect USB and
run `status`; it must report `boot-attempts: 3/3`. Flashing the latest OS slot
and bootloader does not erase boot metadata. Enter `retry` at the recovery
prompt while the serial terminal is connected; the counter is cleared, the
device reboots, and the validated OS can confirm the new attempt.

When upgrading from the Slice 1D pair, flash the Slice 1E bootloader **before**
the Slice 1E OS slot. The new bootloader can launch the older OS while creating
the initial metadata record. The new OS intentionally refuses to run without a
pending record, so flashing it first against the older bootloader would create
a watchdog reboot loop. Use ROM BOOTSEL or `picotool reboot -u -f` between the
two UF2 copies.
