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
- [ ] **Slice 1D:** Transfer control to the validated minimal OS
- [ ] **Slice 1E:** Add watchdog, boot attempts, and boot-success handshake
- [ ] **Slice 1F:** Finalize recovery behavior and reproducible UF2 artifacts
- [x] Establish USB serial logging
- [x] Record reset reason
- [x] Add the initial USB recovery console
- [ ] Establish SWD debugging

Exit condition: the bootloader validates and starts an OS that prints its build
identity, while invalid images enter a recoverable state.

## Milestone 2: PicoCalc bring-up

- [ ] Display diagnostic pattern and terminal renderer
- [ ] Keyboard input and power events
- [ ] SD read-only operation, then safe writes
- [ ] Battery status and controlled shutdown
- [ ] PSRAM test and allocator

Exit condition: an interactive terminal survives repeated cold boots and forced
power interruptions without corrupting its boot metadata.

## Milestone 3: kernel and services

- [ ] Work queues, timers, IPC, and capability checks
- [ ] Device manager and attachment descriptors
- [ ] Filesystem, settings, audit, and engagement-scope services
- [ ] Crash reporting and recovery UI

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
