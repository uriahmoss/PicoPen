# PicoPen Architecture

## System model

PicoPen consists of a small immutable boot stage, an independently updateable OS
image, and data stored on the PicoCalc SD card. The RP2350 ROM bootloader remains
the final recovery mechanism.

```text
RP2350 ROM / BOOTSEL
        |
        v
PicoPen stage-1 bootloader
  - boot metadata
  - image validation
  - rollback decision
  - recovery console
        |
        v
PicoPen kernel
  - scheduler and timers
  - memory ownership
  - message queues
  - capability checks
        |
        +-------------------+
        |                   |
        v                   v
system services         hardware services
  UI/terminal             display/keyboard
  filesystem              SD/PSRAM
  audit log               Wi-Fi/USB/GPIO
  scope policy            attachment buses
        |                   |
        +---------+---------+
                  v
          sandboxed applications
```

## Core boundaries

### Bootloader

The bootloader owns image selection, validation, update finalization, rollback,
and recovery. It does not contain the graphical interface or security tools.

### Kernel

The kernel provides cooperative services first, with preemptive scheduling added
only if driver latency requires it. Applications communicate through versioned
messages rather than sharing driver state.

### Drivers

Drivers expose narrow interfaces and never silently transmit. Radio drivers
separate receive, analyze, and transmit capabilities.

### Services

Long-lived services manage the terminal, storage, networking, attachments,
engagement scope, secrets, and audit records.

### Applications

Applications request named capabilities. Sensitive operations require both an
application grant and a current engagement policy that permits the operation.

## Initial execution model

- Core 0: kernel, services, input, networking, and application execution.
- Core 1: display composition and bounded hardware work queues.
- Internal SRAM: kernel, stacks, DMA buffers, and latency-sensitive state.
- PicoCalc PSRAM: screen buffers, caches, parsers, and application heaps after
  the PSRAM driver is validated.
- Internal flash: bootloader, OS image, boot metadata, and recovery data.
- SD card: tools, documentation, captures, audit exports, and user data.

## API stability

Interfaces carry a major and minor version. No third-party application ABI is
promised before version 0.5. On-disk formats must be documented before version
1.0.
