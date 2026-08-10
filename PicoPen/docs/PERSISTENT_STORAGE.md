# Persistent storage and credential vault

Status: approved development baseline (2026-08-09)

## Flash boundary

PicoPen uses only the existing 192 KiB persistent region at flash offset
`0x003D0000`. The region contains littlefs v2.11.3 with 4 KiB erase blocks,
256-byte program units, bounded static caches, and 48 blocks. Image slots and
boot metadata remain outside this region.

The OS never formats this region automatically. A blank or invalid filesystem
is reported as uninitialized and requires the local **Store > Initialize**
action. A mounted filesystem cannot be reformatted through the current API.

## Wi-Fi vault v1

`/wifi.vlt` is an atomic, fixed-size record. It contains a version, PBKDF2
iteration count, random 128-bit salt, random 96-bit nonce, AES-256-GCM
ciphertext, and a 128-bit authentication tag. SSID and password are inside the
authenticated ciphertext. The header is authenticated as additional data.

The key is derived with PBKDF2-HMAC-SHA-256 (50,000 iterations) from:

- a locally entered PIN of 4-16 printable characters;
- the record's random salt; and
- the Pico's unique board identifier.

The board identifier associates a copied vault with one board but is not a
hardware secret. Security therefore still depends on PIN entropy. OTP, secure
boot fuses, and irreversible configuration remain untouched during
development.

The PIN is required after every reboot. Loading does not automatically enable
Wi-Fi or connect; the operator must separately authorize those actions.
Plaintext credentials exist only in bounded RAM buffers, are masked in the UI,
are omitted from audit/serial output, and are scrubbed after connection. Five
failed decryptions impose a 30-second in-memory lockout. **Forget Saved**
removes the vault through an explicit local action.

## Failure behavior

- Authentication failure returns no partial plaintext.
- Unsupported versions and malformed sizes are rejected.
- Filesystem or atomic-replace failure preserves a denied/error state.
- Corruption never falls back to plaintext storage or an automatic format.
- Power-loss behavior and full-media handling still require physical fault
  testing before this storage baseline is accepted as production-ready.
