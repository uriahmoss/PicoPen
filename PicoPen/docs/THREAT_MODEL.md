# Safety and Threat Model

## Assets

- Boot integrity and recovery availability
- User configuration, engagement records, and secrets
- Evidence and audit-log integrity
- Connected target hardware
- Radio-spectrum compliance
- PicoCalc batteries and external power sources

## Trust boundaries

- Internal flash versus removable SD data
- Kernel/services versus applications
- PicoPen versus attached modules
- Local user input versus imported scripts and packages
- Receive-only operations versus transmission or emulation
- In-scope versus out-of-scope networks and devices

## Required controls

- Validate all executable images and application packages.
- Treat SD content, captures, network traffic, and attachment descriptors as
  untrusted input.
- Enforce memory bounds, timeouts, and maximum record sizes in every parser.
- Require named capabilities for sensitive hardware and network operations.
- Require an active engagement profile before active assessment operations.
- Log sensitive operations with monotonic ordering and exportable timestamps.
- Require physical confirmation for radio transmission, emulation, target power,
  and USB input automation.
- Apply regional frequency and power policy below the application layer.
- Default newly detected attachments to unpowered or receive-only operation.
- Never store secrets in plaintext audit records.

## Engagement scope

A scope profile may name:

- permitted IP addresses and network ranges
- permitted device identifiers
- permitted attachment capabilities
- permitted radio region and bands
- start and expiration times
- operator and engagement reference

Scope checks are enforced by shared services rather than duplicated in each
application. Low-level developer builds remain visibly marked and must not be
represented as enforcing production policy.

## Out of scope for version 1

- Protection against invasive physical attacks on the RP2350 or flash
- Formal verification of the kernel
- Certification as a calibrated RF or electrical measurement instrument
- Automatic proof that the operator has legal authorization
