# Milestone 1 verification evidence

## Reproducible Release artifacts

Two independent `Release` build directories produced byte-identical UF2 files.
SHA-256 digests from the verification run were:

| Artifact | SHA-256 |
|---|---|
| `picopen_bootloader.uf2` | `a3d77a2225c1da0b1127c68e1f3ae8c7b7f12b3ef79db4fed8e93829099aff3b` |
| `picopen_bringup.uf2` | `2e153ea92f32f9bf90df4e93e3a2b5af5eb87ca33054d94f46bc2a608ae3a4cb` |
| `picopen_os.uf2` | `c3c7194b0a30fb33302e49dacc23806644d0213f86cfa70b1aeba3d4a19c9a02` |
| `picopen_os_slot.uf2` | `22a67797ced5e781f030abb31b7f1672a4c9cef74400c48768351b2e0592c3eb` |

The hashes describe the source state completing Slice 1F. Future source,
compiler, SDK, or packaging changes are expected to produce new hashes.

## Hardware acceptance

Physical Pico 2 W testing established that:

- the bounded stage-1 image validates and directly enters the primary OS slot
- the final deterministic OS artifact reaches its persistent heartbeat loop
- a confirmed OS boot resets the consecutive-attempt counter
- three power-cycled, USB-unavailable boots enter recovery at `3/3`
- the local `retry` command clears the lockout and restores validated boot
- the warm recovery request reaches RP2350 ROM BOOTSEL
- the standalone bring-up artifact remains available as a recovery fallback

The final OS test reported version `0.0.1`, primary slot, linked address
`0x10051000`, and continuing heartbeats. The one-time success banner can be
missed when a host terminal attaches after USB enumeration; the heartbeat loop
itself begins only after the metadata confirmation write succeeds.
