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

HTTP inspection sends only a fixed bounded HEAD request. SSH inspection sends
nothing after connection and accepts only a bounded printable banner. TLS
metadata remains disabled unless certificate parsing, peer authentication,
clock validity, entropy, heap limits, and cancellation have a reviewed contract.
Inspector history is volatile and never retains packet payloads or credentials.

Blank targets never authorize discovery. Network-wide activity exists only as
an explicit discovery task constrained to the authorized boundary, with local
preview and confirmation of the target count and rate.

The Security session may omit a global network limit. This does not supply a
blank target to an active tool: each active application still collects and
confirms its target. A configured limit remains an additional service-enforced
restriction in every security mode.

Session-report export is the only planned removable-media write path in the
initial security workbench. It requires a separate capability and local action,
uses bounded typed fields rather than arbitrary application bytes, and excludes
credentials, keys, PINs, raw payloads, and API secrets. Media removal, filename
collision, insufficient space, serialization overflow, sync failure, or an
interrupted write must leave the session available in memory and must not expose
a completed-looking partial report.

SD application packages, manifests, icons, assets, bytecode, saved state, and
catalog metadata remain untrusted input, but unsigned does not mean forbidden.
Owner mode may remember approval for an exact package hash and launch compatible
interpreted apps directly from `/PicoPen/apps`. Parsing, canonical paths,
content hashes, compatibility checks, resource limits, and visible signature
status remain mandatory because they prevent corruption and ambiguity rather
than second-guessing the operator. Applications do not receive secrets,
executable flash, boot/update control, or unrestricted peripheral access merely
because they are locally approved. Resource exhaustion, malformed bytecode, and
crashes must fail within the app boundary while built-in recovery remains
available.

The local owner is trusted to choose tools and targets. The system focuses hard
interlocks on actions whose effects are difficult to undo, easy to trigger
remotely, capable of exposing secrets, or capable of affecting third parties.
Routine local navigation, read-only inspection, SD browsing, and previously
approved bounded app tasks should not accumulate repetitive confirmation steps.

## Out of scope for version 1

- Protection against invasive physical attacks on the RP2350 or flash
- Formal verification of the kernel
- Certification as a calibrated RF or electrical measurement instrument
- Automatic proof that the operator has legal authorization
