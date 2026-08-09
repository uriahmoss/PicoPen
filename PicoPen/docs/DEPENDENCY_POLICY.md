# Dependency Policy

PicoPen accelerates standard platform work with audited, pinned, permissively
licensed dependencies. Dependencies do not define PicoPen security policy.

## Admission requirements

A dependency must:

- use an approved permissive license such as BSD, MIT, or the FatFs license;
- be pinned to an immutable version or commit in `tools/dependencies.lock`;
- appear in the machine-checked `tools/dependencies.json` manifest;
- have its source, version, license, modifications, and purpose recorded in
  `THIRD_PARTY_NOTICES.md`;
- compile with only the features PicoPen needs;
- sit behind a PicoPen-owned interface with bounds, timeouts, ownership, and
  capability checks;
- pass malformed-input, failure-path, and relevant hardware tests; and
- be reviewable and replaceable without changing application authority.

GPL, proprietary, and unclearly licensed PicoCalc projects may be inspected as
behavioral references but their implementation is not copied. A license-policy
change requires explicit user approval and an architecture review.

## Security integration rules

- Filesystem libraries never auto-run SD content and default to read-only.
- Network and USB libraries do not create listening or active interfaces until
  a PicoPen service authorizes them.
- GUI libraries receive sanitized models, not credentials or raw driver state.
- Protocol and capture parsers cannot directly transmit, emulate, power a
  target, or widen engagement scope.
- Upstream examples and default credentials are excluded from production
  images.
- Dependency errors fail closed and are reported through bounded structured
  logs without secrets.

## Update process

Dependency upgrades are explicit roadmap slices. Each upgrade records upstream
changes, license changes, security advisories, binary-size impact, tests, and a
rollback point. No build step fetches or silently upgrades dependencies.
