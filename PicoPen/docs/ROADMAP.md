# Roadmap

## Milestone 0: repository and decisions

- [x] Define product goals, architecture, and feature inventory
- [x] Define ethical-use and hardware safety boundaries
- [ ] Select project license
- [x] Pin the Pico SDK version
- [ ] Record the PicoCalc V2.0 schematic revision
- [ ] Create the verified board pin map
- [x] Decide the first flash layout
- [ ] Select the production signature scheme and key-storage policy

## Milestone 1: first boot

- [x] Add the initial CMake/Pico SDK build
- [x] Add a safe first-flash bring-up target
- [x] **Slice 1A:** Define the boot contract, flash map, and image format
  - [x] Confirm the Pico SDK board definition uses 4 MiB of flash
  - [x] Allocate erase-aligned boot, metadata, image, and persistent regions
  - [x] Define the fixed-size v1 image header and validation order
  - [x] Document development policy, recovery behavior, and handoff boundary
- [x] **Slice 1B:** Build bootloader and OS as separate link targets
  - [x] Link `picopen_bootloader` only inside the 256 KiB stage-1 region
  - [x] Link `picopen_os` at the primary slot payload address
  - [x] Produce independent ELF, BIN, HEX, map, disassembly, and UF2 artifacts
  - [x] Verify the OS UF2 block range remains inside its contracted slot
  - [x] Preserve the existing `picopen_bringup` target
- [x] **Slice 1C:** Validate the OS header, bounds, and SHA-256 digest
  - [x] Generate a deterministic manifest-wrapped development image
  - [x] Package a complete primary-slot UF2 at the contracted address
  - [x] Reject malformed identity, flags, policy, and noncanonical fields
  - [x] Reject incompatible versions and all out-of-slot bounds
  - [x] Validate the payload with RP2350 hardware SHA-256
  - [x] Report validation status without transferring control
  - [x] Test valid packaging, corruption, overflow, and forged header CRC
- [x] **Slice 1D:** Transfer control to the validated minimal OS
  - [x] Restrict the ROM chain window to the SHA-256-validated payload
  - [x] Test and reject RP2350 ROM chaining for the partitionless slot format
  - [x] Implement bounded Cortex-M33 vector-table handoff
  - [x] Confirm the SDK `IMAGE_DEF` lies in the first 4 KiB of the payload
  - [x] Declare the relocated OS vector table in its RP2350 `IMAGE_DEF`
  - [x] Preserve recovery mode if validation or ROM chaining fails
  - [x] Report the chained-boot flag from the minimal OS
  - [x] Build the bootloader, manifested slot image, and legacy bring-up image
  - [x] Pass host packaging and malformed-image tests
  - [x] Keep USB uninitialized before ROM chaining so the OS owns USB cleanly
  - [x] Use a watchdog-scratch handshake for clean warm ROM BOOTSEL entry
  - [x] Verify warm ROM BOOTSEL entry on the physical Pico 2 W
  - [x] Add bounded handoff watchdog and recovery checkpoint reporting
  - [x] Reset USBCTRL before the chained OS initializes TinyUSB
  - [x] Verify bootloader-to-OS transfer on the physical Pico 2 W
    - Hardware evidence: the primary-slot OS printed version `0.0.1`, linked
      address `0x10051000`, and a live heartbeat after validated transfer.
- [x] **Slice 1E:** Add watchdog, boot attempts, and boot-success handshake
  - [x] Accept a three-consecutive-attempt recovery policy
  - [x] Define a versioned 256-byte metadata record with CRC-32
  - [x] Select the newest valid A/B generation with wraparound handling
  - [x] Write the alternate metadata sector and verify it before transfer
  - [x] Persist a pending attempt before entering the OS
  - [x] Confirm boot and reset attempts after core runtime and USB are ready
  - [x] Enter recovery after three unconfirmed attempts or a metadata error
  - [x] Add a local `retry` command that safely clears an attempt lockout
  - [x] Pass host tests for corruption, interrupted writes, and generation wrap
  - [x] Verify success confirmation and three-failure recovery on hardware
    - Hardware evidence: the OS confirmed a primary-slot boot and continued
      heartbeats; three power-cycled USB-unavailable attempts entered recovery
      at `3/3`, and `retry` restored a successful validated boot.
- [x] **Slice 1F:** Finalize recovery behavior and reproducible UF2 artifacts
  - [x] Remove the nondeterministic SDK build-date record from firmware
  - [x] Add UF2 structure, RP2350 metadata/family, and region verification
  - [x] Prove byte-identical clean release builds
    - Two independent Release directories produced identical bootloader,
      bring-up, raw OS, and manifested OS-slot UF2 SHA-256 hashes.
  - [x] Record final recovery acceptance evidence and artifact hashes
    - See [MILESTONE_1_EVIDENCE.md](MILESTONE_1_EVIDENCE.md).
- [x] Establish USB serial logging
- [x] Record reset reason
- [x] Add the initial USB recovery console
- [x] Harden bring-up recovery to fit entirely inside the 256 KiB boot region
  - [x] Remove CYW43 firmware and onboard LED commands from recovery
  - [x] Enforce the recovery flash boundary in the linker
  - [x] Add a two-reset watchdog-scratch path for warm BOOTSEL entry
- [ ] Establish SWD debugging

Exit condition: the bootloader validates and starts an OS that prints its build
identity, while invalid images enter a recoverable state.

## Milestone 2: PicoCalc bring-up

- [x] **Slice 2A:** Verify the PicoCalc V2.0 board contract
  - [x] Pin the official schematic and reference-firmware revision
  - [x] Transcribe and cross-check display and keyboard pin assignments
  - [x] Centralize the draft pins without activating GPIO
  - [x] Confirm `CPI 2.0` on the physical mainboard marking
- [x] Display diagnostic pattern and terminal renderer
  - [x] Add a bounded blocking SPI1 display driver using the verified pins
  - [x] Add a fixed synthwave color-band and alignment-grid diagnostic
  - [x] Verify the diagnostic pattern on the physical CPI 2.0 display
    - Hardware evidence: all synthwave color bands and the gold alignment grid
      rendered correctly across the installed 320 x 320 panel.
  - [x] Add the terminal glyph renderer and bounded dirty-region updates
    - [x] Define a 40 x 20 fixed-cell terminal and synthwave palette
    - [x] Add a repository-owned boot-console glyph subset
    - [x] Add bounded newline, carriage-return, wrap, and dirty-cell rendering
    - [x] Verify the terminal diagnostic on the physical CPI 2.0 display
      - Hardware evidence: all expected boot-console lines rendered legibly,
        correctly oriented, and within the 320 x 320 panel bounds.
- [x] Keyboard input and power events
  - [x] Add bounded I2C1 identity and FIFO reads with 20 ms timeouts
    - Probe the read-only version register at 10, 100, then 400 kHz because
      official ClockworkPi applications use both ends of that range.
    - On a NACK, report idle SDA/SCL levels and perform one bounded read-only
      scan of non-reserved 7-bit addresses; do not issue controller commands.
  - [x] Keep backlight, reset, and power-off registers write-disabled
  - [x] Echo pressed printable keys and identify special keys on the terminal
  - [x] Verify controller firmware identity and key events on hardware
    - Hardware evidence: after restoring the official ClockworkPi keyboard
      BIOS v1.6, the controller acknowledged on I2C1 and physical key presses
      rendered correctly in the PicoPen terminal.
    - Recovery evidence: the pre-recovery bus was electrically idle
      (`SDA=1`, `SCL=1`) with no responding 7-bit address, distinguishing a
      stopped controller from a Pico-side pin or protocol failure.
- [ ] SD read-only operation, then safe writes
  - [x] Accept and centralize the CPI 2.0 SPI0 pin contract
  - [x] Identify an inserted card using bounded, read-only initialization
    - Block write, erase, formatting, and filesystem operations are absent.
    - Initial hardware result was `no-response`, `R1=0xFF` while the STM32
      keyboard/power controller was not responding on I2C and the
      PMIC-controlled SD supply was unavailable.
    - Add a bounded mode-0 software-SPI fallback for startup, matching the
      low-speed approach in ClockworkPi's PicoMite path.
    - Align card detect, 8 mA output drive, MISO hysteresis, CRC7, response
      window, and glitch-free CS initialization with ClockworkPi references.
    - Use a standards-compatible 400 kHz startup clock, selected-card ready
      wait, and CS-high release clocks consistent with working PicoCalc
      PicoMite, Picoware, and MicroPython implementations.
    - Honor the official 1.5-second post-detect stabilization delay for the
      SD socket's PMIC-controlled ALDO1 supply.
    - Hardware evidence: after recovering the official keyboard BIOS v1.6 and
      confirming `KBD FW: 0X16 READY 10K`, PicoPen identified the inserted card
      over hardware SPI as SDHC/SDXC v2 with card detect active and OCR
      `0xC0FF8000`.
  - [x] Read and classify boot sectors without mounting a filesystem
    - Read only LBA 0 and, when present, the first MBR partition boot sector.
    - Bound command response and data-token waits to 500 ms.
    - Recognize FAT12/16/32 and exFAT signatures; expose no write, erase,
      formatting, or mount interface.
    - Hardware evidence: the CPI 2.0 unit read the first partition boot sector,
      identified FAT at LBA 2048, and continued accepting keyboard input.
- [ ] Battery status and controlled shutdown
  - [x] Read and display bounded battery percentage and charging state
    - Hardware evidence: the accelerated baseline booted with the keyboard,
      storage, battery diagnostic, and interactive terminal functioning.
  - [ ] Add a locally confirmed controlled-shutdown path
- [ ] PSRAM test and allocator

Exit condition: an interactive terminal survives repeated cold boots and forced
power interruptions without corrupting its boot metadata.

## Milestone 3: kernel and services

- [x] **Slice 3A:** Establish the dependency and secure-service baseline
  - [x] Accept a permissive-license-only dependency policy
  - [x] Keep the PicoPen lightweight kernel and PicoPen-owned policy boundaries
  - [x] Define deny-by-default capabilities and secure failure behavior
  - [x] Add automated dependency provenance and license checks
- [x] **Slice 3B:** Integrate pinned FatFs R0.15 in read-only mode
  - [x] Compile out create, write, delete, rename, format, and free-space
    mutation
  - [x] Route block reads through the bounded PicoPen SD service
  - [x] Treat names, directory entries, partition data, and file contents as
    untrusted and enforce path, depth, size, and iteration limits
  - [x] List a physical card's root directory without auto-running content
    - Hardware evidence: the CPI 2.0 unit mounted the existing FAT card
      read-only and the bounded `ls` command listed root entries without
      disrupting keyboard input.
- [ ] **Slice 3C:** Add the versioned storage service
  - Separate removable read access, evidence export, and privileged mutation
  - Require explicit capability and local policy for every SD write
  - Add safe-removal, media-change, timeout, and corruption reporting
- [ ] **Slice 3D:** Add pinned littlefs for internal settings and audit journals
  - Keep secrets in a separately designed encrypted vault
  - Test interrupted updates and full-media behavior
- [ ] Work queues, timers, IPC, and capability checks
  - [x] Add a fixed-capacity deadline queue with per-loop execution budgets
  - [x] Add named deny-by-default capability evaluation
  - [ ] Add versioned IPC messages and service ownership
- [ ] Device manager and attachment descriptors
- [ ] Filesystem, settings, audit, and engagement-scope services
- [ ] Crash reporting and recovery UI
- [ ] Structured audit service
  - [x] Add a bounded monotonic in-memory audit ring with no secret payloads
  - [ ] Persist authenticated audit records after the littlefs layout is
    reviewed
- [ ] Interactive terminal baseline
  - [x] Add bounded `help`, `status`, `devices`, `ls`, `security`, and `audit`
    commands
  - [x] Verify commands and keyboard responsiveness on hardware
  - [x] Verify bottom-row scrolling and whole-word wrapping on hardware
    - Hardware evidence: the accelerated command baseline operated on the
      physical unit; the follow-on terminal build scrolls at row 20 and wraps
      complete words when they fit within the 40-column line.
- [ ] Integrate pinned LVGL behind the PicoPen compositor and input services
- [ ] Use Pico SDK-pinned TinyUSB and networking stacks behind capability checks

## Milestone 4: safe hardware workbench

- [ ] GPIO and UART tools
- [ ] I2C and SPI workbenches
- [ ] 1-Wire enumeration
- [ ] IR receive/decode and confirmed transmit
- [ ] Logic capture proof of concept

## Milestone 5: networking

- [ ] Wi-Fi management and network diagnostics
- [ ] Scoped TCP service identification
- [ ] HTTP and TLS inspection
- [ ] SSH client
- [ ] BLE inventory

## Milestone 6: modular RF and NFC

- [ ] PN532 test-tag support
- [ ] CC1101 receive and offline decode
- [ ] Regional policy database and transmit interlock
- [ ] Field Dock Revision A design and validation

## Milestone 7: PicoLink remote and AI bridge

- [ ] Define a versioned, framed PicoPen bridge protocol
- [ ] Implement USB CDC transport with request IDs and bounded messages
- [ ] Add device pairing, session authentication, and expiration
- [ ] Implement offline, observe, assisted, and scoped-automation modes
- [ ] Enforce engagement scope and capabilities independently on PicoPen
- [ ] Add local approval prompts and a persistent remote-session indicator
- [ ] Add operation status, cancellation, timeout, and emergency-stop handling
- [ ] Stream structured results with audit-record references
- [ ] Implement privacy filters, payload limits, and preview-before-upload
- [ ] Build a trusted-host PicoLink bridge daemon
- [ ] Expose narrow PicoPen operations through an MCP server for Codex
- [ ] Add an OpenAI Responses API integration through the trusted-host bridge
- [ ] Keep API credentials off PicoPen and out of the repository
- [ ] Add authenticated Wi-Fi transport after the USB implementation is stable
- [ ] Add the on-device Assistant application and plan-approval UI
- [ ] Test connection loss, replay resistance, cancellation, and scope expiry

Exit condition: an authorized Codex or API-backed agent can inspect PicoPen,
propose a bounded operation, receive local approval where required, execute only
within the active scope, stream structured results, and be stopped locally at
any time. PicoPen remains safe and useful when the bridge is unavailable.

## Milestone 8: application SDK and release

- [ ] Lua runtime and capability-limited APIs
- [ ] Package format and signing
- [ ] Reproducible release pipeline
- [ ] Hardware-in-the-loop regression fixture
- [ ] Version 1.0 security and documentation review
