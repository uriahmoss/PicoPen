# Contributing to PicoPen

PicoPen is currently in early bring-up. Discuss architectural changes before
submitting a large implementation.

## Contribution principles

- Preserve the ROM recovery path and never weaken boot validation silently.
- Include bounds checks and failure behavior for all untrusted input.
- Keep hardware transmissions disabled by default.
- Document voltage, current, timing, and regional assumptions.
- Add host-side tests for parsers and policy logic.
- Add hardware-in-the-loop instructions for driver changes.
- Do not add credential theft, persistence, destructive payload, evasion, or
  unauthorized-access features.

## Commit expectations

- Keep commits focused and describe the hardware tested.
- Record the PicoCalc and core-board revisions used.
- Do not commit private keys, wireless credentials, captures, or client data.
- Format and static-analysis requirements will be added with the build system.

## License note

The project license has not yet been selected. External contributions should not
be accepted until that decision is recorded.
