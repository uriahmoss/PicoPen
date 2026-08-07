# Feature List

Status markers used here:

- **MVP**: required for the first useful release
- **Planned**: part of the intended product
- **Research**: feasibility or hardware work remains

## Platform and reliability

- **MVP** Recoverable boot with image validation and failure rollback
- **MVP** Watchdog, reset-reason reporting, and bounded crash records
- **MVP** Monotonic clock, timers, work queues, and multicore coordination
- **MVP** Structured serial logging
- **Planned** Signed releases and reproducible build metadata
- **Planned** SD and network update packages
- **Planned** Fast suspend/resume where the PicoCalc power hardware permits it

## PicoCalc hardware

- **MVP** 320 x 320 display with partial redraws and terminal rendering
- **MVP** Keyboard input, modifiers, repeat, and backlight control
- **MVP** SD card with safe removal and corruption-aware recovery
- **MVP** Battery state and controlled shutdown
- **Planned** PSRAM allocator and cache
- **Planned** Audio notification service
- **Planned** Brightness and power profiles

## User interface

- **MVP** Fast text console and command palette
- **MVP** File browser and text viewer
- **MVP** Settings, device inventory, and engagement-scope editor
- **Planned** Text editor with syntax highlighting
- **Planned** Split terminal and structured result views
- **Planned** Searchable offline reference library
- **Planned** Accessible color and large-text themes

## Attachment framework

- **MVP** Versioned attachment descriptor and device discovery
- **MVP** SPI, I2C, UART, GPIO, PWM, ADC, and 1-Wire services
- **MVP** Per-device current and voltage declarations
- **MVP** Capability grants for receive, transmit, probe, and storage access
- **Planned** Hot-plug notifications where electrically safe
- **Planned** Field Dock carrier-board support
- **Planned** Module firmware update protocol

## Hardware and protocol workbench

- **MVP** GPIO read/write with explicit direction and voltage warnings
- **MVP** UART terminal and capture export
- **MVP** I2C discovery and register inspection
- **MVP** SPI transaction workbench
- **MVP** 1-Wire enumeration using harmless test devices
- **Planned** IR capture, decode, save, and authorized transmit
- **Planned** Low-rate logic capture using RP2350 PIO and DMA
- **Planned** SWD identification and debugging helpers
- **Research** Protected target-power measurement and current monitoring

## Networking

- **MVP** Wi-Fi association and saved-network management
- **MVP** IPv4/IPv6 configuration, ping, DNS, and route diagnostics
- **MVP** Scoped TCP connection testing and service identification
- **MVP** HTTP request/response and TLS certificate inspection
- **Planned** SSH client
- **Planned** Packet capture of traffic visible to the local network stack
- **Planned** Bluetooth Low Energy inventory and GATT inspection
- **Research** External monitor-capable Wi-Fi coprocessor

## NFC, RFID, and radio attachments

- **Planned** PN532 tag inventory and standards-compliant test-tag operations
- **Planned** CC1101 receive, signal metadata, raw capture, and offline decoding
- **Planned** Region-locked CC1101 transmission for authorized test signals
- **Planned** External 125 kHz reader integration
- **Research** ST25R3916-class NFC attachment
- **Research** Intelligent Proxmark-compatible USB/serial integration
- **Research** LoRa, nRF24L01+, and nRF52840 attachment profiles

## Data and reporting

- **MVP** Engagement profiles with explicit network and device scope
- **MVP** Append-only action audit log with export
- **MVP** Timestamped notes and evidence hashes
- **Planned** Capture metadata and chain-of-custody export
- **Planned** Human-readable assessment report generation
- **Planned** Encrypted secrets and configuration vault

## Application platform

- **Planned** Signed native applications
- **Planned** Lua automation with capability restrictions
- **Planned** Stable filesystem and message APIs
- **Planned** Package manifest, dependency declarations, and resource limits
- **Research** WebAssembly application runtime

## Explicit exclusions

PicoPen will not ship modules for credential theft, covert persistence,
destructive actions, unauthorized access, stealth/evasion, or automatic replay
of dynamic access credentials. Raw hardware primitives remain governed by scope,
physical confirmation, and regional radio policy.
