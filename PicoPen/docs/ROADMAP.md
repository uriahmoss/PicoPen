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

## Accelerated delivery bundles

PicoPen groups independent, conventional OS mechanisms into larger hardware
test images. A bundle may cross milestone headings when its components share no
unresolved security or hardware dependency. Signing policy, flash-layout
changes, persistent secrets, storage mutation, and active transmission remain
review gates and are never implied by acceleration.

- [ ] **Bundle A: standalone graphical platform**
  - Battery-only, USB-only, and mixed-power boot without cable-order rituals
  - Optional hot-pluggable USB serial instead of a boot dependency
  - Bounded peripheral readiness, retry, and dynamic device-state updates
  - Partial graphical redraws and measured input latency
  - Synthwave System, Files, status, dialog, and recovery screens
  - Battery warnings, backlight control, and locally confirmed shutdown
  - Data-only menu skins with preview, accessibility checks, and safe fallback
- [ ] **Bundle B: passive security workbench**
  - Engagement-scope editor and persistent scope/security indicators
  - Receive-only GPIO, ADC, I2C, SPI, UART, and attachment inventory
  - Bounded jobs, progress, cancellation, errors, and structured audit events
  - Read-only directory traversal, metadata, text, and hexadecimal viewers
  - No GPIO drive, target power, emulation, probing, or transmission
- [ ] **Bundle C: Wi-Fi and update transport foundation**
  - Wi-Fi disabled by default with locally approved connection management
  - Passive access-point inventory and interface diagnostics
  - Locked online update and both locally launched portal modes
  - Bounded package streaming and shared validation interfaces
  - No installation until signature and inactive-slot contracts are approved
- [ ] **Bundle D: signed and recoverable software update**
  - Approved package signature, key rotation, anti-rollback, and flash layout
  - Online signed-manifest checking and locally approved download
  - Current-Wi-Fi and private-network upload portals
  - Inactive-slot staging, independent boot validation, and automatic rollback

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
  - [x] Confirm boot and reset attempts after core runtime is ready
    - USB was required during initial Slice 1E hardware validation; Slice 2B
      removes it from the success condition so battery-only boot can succeed.
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
  - [x] Add a locally confirmed controlled-shutdown path
    - The keyboard-controller request requires a second explicit local command
      within 15 seconds and uses the controller's bounded six-second delay.
    - Hardware power-off verification remains part of the next baseline test.
- [ ] PSRAM test and allocator
- [ ] **Slice 2B:** Make PicoCalc boot and peripheral discovery power-order safe
  - [x] Remove USB CDC connection as an OS boot-success requirement
  - [x] Allow USB serial presence to remain independent of device readiness
  - [x] Show bounded graphical boot progress while board services stabilize
  - [x] Retry keyboard discovery for a bounded five-second readiness window
  - [x] Initialize battery and SD only after keyboard/PMIC readiness
  - [x] Recheck controller health and recover failed peripherals in the runtime
  - [x] Update shell, GUI, audit, and device-manager state after recovery
  - [ ] Verify battery-only, USB-only, warm-reset, cold-boot, and cable hot-plug paths
  - [ ] Eliminate the full-power-removal and delayed-USB workaround on hardware

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
  - [x] Add versioned IPC messages and service ownership
    - Service registrations, rather than untrusted callers, own capability
      requirements; a boot-time self-test verifies privileged denial.
- [ ] Device manager and attachment descriptors
  - [x] Add a fixed-capacity device inventory and policy-visible states
  - [ ] Parse and validate versioned attachment descriptors
- [ ] Filesystem, settings, audit, and engagement-scope services
  - [x] Add bounded root-only removable-file reads behind `storage.read`
  - [x] Add an inactive-by-default, reference-and-expiry engagement state
  - [ ] Add reviewed state-transition and persistence interfaces
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
  - [x] Add passive `scope` and `workbench` policy/status commands
- [ ] Keyboard-driven GUI baseline
  - [x] Add bounded Home and System menu state machines
  - [x] Add keyboard focus navigation using the official CPI key codes
  - [x] Add Status, Devices, Workbench, Audit, and Security screens
  - [x] Add selectable read-only root file browsing and bounded viewing
  - [x] Keep the command shell as an Advanced Terminal application
  - [x] Add a persistent Cancel/Power Off confirmation without a typing timer
  - [x] Add a locked Wi-Fi Update screen that cannot enable networking
  - [ ] Verify GUI navigation and controlled power-off on hardware
  - [ ] Replace text tiles with compositor-drawn synthwave widgets
    - [x] Add bounded scaled pixel-text and reusable panel primitives
    - [x] Convert Home to direct synthwave panels, icons, and focus borders
    - [x] Redraw only changed Home tiles and use a focus-exclusive color
    - [ ] Convert System, Files, dialogs, and detail screens
- [ ] **Slice 3E:** Complete the responsive graphical platform bundle
  - Add reusable screen stack, panels, lists, dialogs, progress, and notices
  - Remember the last selection independently for each menu and file list
  - Coalesce held navigation events and measure key-to-focus latency
  - Evaluate 100 and 400 kHz keyboard operation with safe fallback
  - Redraw only changed widgets during navigation and background updates
  - Add graphical device-recovery, crash-summary, and degraded-mode screens
  - Add idle timeout, backlight settings, battery warning, and power controls
  - Preserve Advanced Terminal as a policy-equivalent application
  - Add `System > Appearance > Skins`
    - Define a versioned, fixed-size, data-only skin schema with no executable
      code, scripts, arbitrary drawing commands, or direct hardware access
    - Centralize background, panel, focus, warning, success, muted, header,
      footer, border, spacing, and text-scale tokens
    - Ship built-in Synthwave, High Contrast, and Minimal Dark skins
    - Preview without persistence and provide explicit Apply, Cancel, and
      Restore Default actions
    - Enforce minimum contrast, bounds, supported token values, and complete
      fallback to the built-in safe skin when validation fails
    - Keep imported SD skins disabled until the bounded parser and provenance
      policy are reviewed; treat every imported skin as untrusted data
    - Keep persistent selection disabled until the littlefs settings layout is
      approved and power-loss behavior is tested
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
  - Keep Wi-Fi disabled until locally enabled
  - Add saved-network selection without plaintext credentials or logs
  - Add passive access-point, signal, channel, and interface status views
  - Separate connection, passive observation, and active probing capabilities
- [ ] Secure Wi-Fi software update
  - [ ] Approve the production signature and key-rotation policy
  - [ ] Approve an inactive update-slot and rollback flash layout
  - [ ] Define one signed package format shared by every update transport
    - Include format version, hardware target, OS version, security rollback
      version, payload size, SHA-256 digest, release metadata, and signature
    - Never treat a raw UF2 upload as an authorized update
  - [ ] Add online update mode
    - Fetch a bounded signed manifest over authenticated TLS
    - Permit automatic checking but default installation to manual approval
    - Verify board, version, bounds, digest, and signature before staging
  - [ ] Add locally launched portal on the current known Wi-Fi network
    - Display the exact local address and a random single-use pairing code
    - Bind only to the local interface; never enable UPnP or port forwarding
    - Allow one authenticated client and expire after a bounded interval
    - Warn the user to activate this mode only on a trusted local network
  - [ ] Add locally launched private update-network portal
    - Create an isolated temporary access point with a random password
    - Display network, address, pairing code, and expiration locally
    - Require no internet connection and stop the access point after use
  - [ ] Harden both portal modes
    - Start only from the local Software Update menu
    - Rate-limit authentication and reject cross-site browser requests
    - Stream one size-limited package in bounded chunks to the inactive slot
    - Expose no shell, filesystem browser, or general configuration endpoint
    - Shut down on completion, cancellation, timeout, or client-policy failure
  - [ ] Require separate local approval before download, staging, and reboot
  - [ ] Record sanitized update status and audit references without secrets
  - [ ] Let the bootloader independently validate and roll back failed boots
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
