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

### Security invariants

- The bootloader validates every executable image before transfer; removable
  storage is never an implicit boot or trust source.
- Applications receive no ambient hardware, network, filesystem, secret, or
  remote-control authority. Every privileged operation crosses a PicoPen-owned
  capability-checked service boundary.
- Passive observation and active transmission are separate capabilities.
  Active radio, USB HID, target power, GPIO drive, emulation, and automation
  additionally require current engagement scope and local confirmation.
- Imported libraries are mechanisms, not policy authorities. They cannot grant
  capabilities, interpret engagement scope, access secrets directly, or bypass
  the audit service.
- Untrusted parsers run with fixed input limits, deadlines, explicit ownership,
  and no direct transmitter access. A parser failure must fail closed without
  disabling recovery or audit controls.
- Developer mode is visibly marked, never silently enabled, and cannot be used
  to represent production policy enforcement.

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

The networking service owns CYW43 and lwIP polling, association deadlines,
DHCP state, and sanitized local diagnostics. The baseline compiles out TCP,
sockets, and netconn APIs; adding a listener requires a separately reviewed
service contract and explicit local activation. Passive scan selection and IP
diagnostics do not grant probing or remote-control capabilities.

The internal settings service owns the reserved littlefs region. Non-secret
preferences are versioned and checksummed, secrets use the separately
authenticated vault format, and watchdog recovery records are fixed-size.
Atomic replacement is the only general settings mutation primitive.

The primary local interface is a keyboard-driven GUI state machine. It uses
bounded static screens and direct service calls through PicoPen policy checks;
the command shell remains an explicitly selected advanced application. A later
compositor may improve visual fidelity without changing navigation or service
authorization semantics.

### Appearance renderer boundary

The GUI state machine owns navigation, screen data, authorization results, and
input handling. It does not define the geometry or visual components of a
built-in skin. Each trusted built-in skin has an independent renderer module
which owns its complete visual language, including:

- screen and menu layout;
- damage regions and repaint policy;
- fonts and packed bitmap assets;
- icon silhouettes and control shapes;
- borders, shading, textures, colors, and focus treatment; and
- the presentation of submenus, dialogs, status, and help affordances.

Renderers consume bounded, non-owning view models from the GUI and cannot make
navigation or policy decisions. Synthwave geometry is not a base layout that
other renderers must inherit. A built-in renderer may share low-level bounded
pixel and text primitives, but not widgets or layout rules unless both skins
explicitly choose to use them.

Imported skins remain a separate future feature. They are untrusted data and
must use a versioned, bounded declarative format; they never supply executable
renderer callbacks.

### Applications

Applications request named capabilities. Sensitive operations require both an
application grant and a current engagement policy that permits the operation.

## Trusted computing base

The minimal trusted computing base consists of the RP2350 ROM, PicoPen stage-1,
the kernel, capability and scope enforcement, secrets handling, audit service,
and the narrow hardware-service entry points. GUI toolkits, filesystems,
protocol decoders, network clients, and application runtimes remain outside the
policy core even when they execute in the same address space during early
releases. Their requests are validated again at the service boundary.

RP2350 privilege separation and TrustZone remain planned hardening layers. The
capability model must not depend on those layers to fail closed.

## Dependency strategy

PicoPen uses pinned permissively licensed components to accelerate standard
mechanisms while retaining PicoPen-owned policy and interfaces. GPL or
unclearly licensed firmware may be used as behavioral reference material but
is not copied into PicoPen. See [DEPENDENCY_POLICY.md](DEPENDENCY_POLICY.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

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
