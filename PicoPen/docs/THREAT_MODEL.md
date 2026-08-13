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
- Deny sensitive operations when capability, scope, clock validity, audit
  availability, or required physical confirmation cannot be established.
- Keep credentials and signing keys out of removable storage by default.
  Wi-Fi vault v1 uses authenticated encryption with a PIN-derived key associated
  with the unique board identifier; the identifier is not a hardware secret,
  so offline resistance continues to depend on PIN entropy. See
  `PERSISTENT_STORAGE.md`.
- Authenticate remote-control sessions, bind each request to an expiring
  session and engagement profile, reject replays, and retain a local emergency
  stop that remote software cannot override.
- Verify signed application and update manifests before parsing optional
  payloads or allocating attacker-controlled sizes.
- Rate-limit authentication, network probing, radio transmission, attachment
  enumeration, and malformed-input logging.
- Scrub secret buffers and avoid exposing raw credentials, keys, or sensitive
  captures through crash reports, serial logs, UI previews, or remote bridges.

## Secure defaults

- Radios, USB HID, GPIO outputs, target power, and attachment transmitters start
  disabled.
- SD volumes mount read-only until an explicit storage policy authorizes a
  bounded write operation; executable content is never auto-run.
- Network services do not listen by default. Wi-Fi association does not grant
  scan, probe, capture, or remote-control authority.
- Newly installed applications have no capabilities. Grants are explicit,
  reviewable, revocable, and narrower than the engagement scope.
- Expired or absent engagement profiles permit passive local inspection only.
- Recovery mode exposes repair and status operations, not secrets or assessment
  tools.

## Security failure behavior

Failure of scope validation, audit persistence, authentication, package
verification, or a privileged service returns a bounded error and leaves the
active mechanism disabled. Security failures must not fall back to an
unrestricted developer path.

## Engagement scope

A scope profile may name:

- permitted IP addresses and network ranges
- permitted hostnames and TCP/UDP port ranges
- permitted device identifiers
- permitted attachment capabilities
- permitted radio region and bands
- start and expiration times
- operator and engagement reference

Scope checks are enforced by shared services rather than duplicated in each
application. Low-level developer builds remain visibly marked and must not be
represented as enforcing production policy.

Active recon is deliberately single-target and single-request. It has no range
scanner, listener, stealth mode, payload delivery, credential use, or exploit
path. The service revalidates scope while a job is running and cancels work when
the scope expires or is revoked.

## Out of scope for version 1

- Protection against invasive physical attacks on the RP2350 or flash
- Formal verification of the kernel
- Certification as a calibrated RF or electrical measurement instrument
- Automatic proof that the operator has legal authorization
