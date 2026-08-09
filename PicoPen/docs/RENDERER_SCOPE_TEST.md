# Renderer-Owned Scope Indicator Test

This corrective fast-track removes the generic Home-screen scope badge. Each
trusted theme renderer now receives semantic scope state and draws one native
indicator. It also remembers the selected root Files entry when leaving and
reopening Files.

## Flash

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` using the established PicoPen
OS-slot procedure.

## Acceptance checks

1. With Synthwave active and no scope, confirm Home shows exactly one
   `SCOPE OFF` label in the header. There must be no rectangle or second label
   covering the original text.
2. Activate a valid scope under **System > Security**, then return Home.
   Confirm the same native label changes cleanly to `SCOPE ON`; `SESSION`
   replaces `LOCKED` below it.
3. End the scope and confirm the native label returns to `SCOPE OFF` and
   `LOCKED` without leaving old pixels behind.
4. Select Crayon and repeat the off/on check. Confirm there is one handwritten
   scope label and no Synthwave-style rectangular overlay.
5. In Files, move to a root entry, press Escape to Home, and reopen Files.
   Confirm the same valid root entry remains selected.
6. Confirm Home focus navigation still redraws only the old and new items and
   does not disturb the scope label.

No capability, radio, storage-write, or hardware-interface behavior changes in
this corrective image.

## Hardware result

Synthwave passed with one clean native indicator. Crayon changes state and no
longer receives the generic overlay, but its native indicator still requires
visual refinement. That work is recorded in `docs/IMPROVEMENTS.md` and is not
treated as visually accepted.
