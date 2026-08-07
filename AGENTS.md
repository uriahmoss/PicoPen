# Project Guidelines

The clean-code and working-practice guidance in this file was inherited from
the Pentest Scripts project. PicoPen-specific rules later in this file take
precedence wherever firmware development differs from the original script
project.

Assume standalone penetration-testing scripts target Kali Linux unless stated
otherwise. This assumption does not apply to PicoPen firmware, build tools, or
host-side tests.

## Clean Code

Follow clean-code principles and rules, but do not feel constrained by them. Use these general guidelines:

- Avoid nesting `if`, `for`, and other conditional or looping statements where practical.
- Keep code, classes, functions, and related constructs as concise as possible.
- Separate unrelated logic into its own file, class, helper function, or library.
- Avoid magic numbers and hard-coded values.
- When appropriate, use descriptive, reusable enums scoped as needed, such as within a class, function, or project.
- When code encountered within the current scope of work breaks these rules, refactor it to comply.

## Working Practices

- If you have a question, do not assume; ask for clarification.
- Do not make large architectural decisions without user input.
- Do not take the lazy way out.
- Update, run, and verify unit tests when applicable.
- Work in small slices. If a piece of work is too large, divide it into smaller pieces and backlog the remainder for later.
- Use logging throughout the project:
  - Log normal project flow at the `info` level.
  - Log minor issues or inconsistencies at the `warning` level.
  - Log major problems or dead ends at the `error` level.
- To reiterate: ask for clarification rather than making an unsupported assumption.

## Script Organization

These inherited rules apply only to a future standalone script collection, not
to PicoPen firmware or its repository tooling:

- Store standalone penetration-testing scripts under a dedicated application or tooling directory selected in the architecture.
- Organize scripts into subfolders based on the primary program they use.
- Create a program subfolder only when its first script is added; do not create unused placeholder folders.
- Use readable, descriptive script names that clearly communicate what each script does.
- Prefer lowercase kebab-case filenames unless the relevant language or tool has a stronger convention.

## PicoPen Firmware Rules

### Scope and target

- The active project is `PicoPen/`, targeting Raspberry Pi Pico 2 W (`pico2_w`, RP2350 ARM secure platform) in a ClockworkPi PicoCalc.
- Use the repository-pinned Pico SDK and dependency versions. Do not silently upgrade the SDK, compiler, CMake generator, picotool, or pioasm.
- Treat `PicoPen/docs/ARCHITECTURE.md`, `PicoPen/docs/BOOT_CONTRACT.md`, `PicoPen/docs/THREAT_MODEL.md`, and `PicoPen/docs/ROADMAP.md` as design constraints.
- Centralize board pins and flash-layout constants. Do not duplicate hardware addresses, region sizes, or GPIO assignments as magic numbers.

### Build and verification

- Configure Debug builds with `cmake --preset pico2w-debug` and build with `cmake --build --preset pico2w-debug`.
- Preserve the `picopen_bringup` target until the complete boot chain has been verified on hardware.
- Build every affected firmware target and run relevant host tests before marking a slice complete.
- Use `python -m unittest discover -s tests -p "test_*.py"` for the current host test suite.
- Treat compiler warnings, linker-region overflow, malformed artifacts, failed tests, and `git diff --check` findings as failures that must be resolved before committing.
- Verify generated UF2 address ranges whenever linker placement or packaging changes.

### Boot and hardware safety

- Preserve RP2350 ROM BOOTSEL as the final recovery mechanism.
- Do not program OTP, disable debugging, enable secure-boot fuses, or perform other irreversible device configuration during development.
- Do not flash boot-chain artifacts until the current roadmap slice explicitly produces a complete and reviewed boot path. Until Slice 1D passes, continue using `picopen_bringup.uf2` for hardware testing.
- Never transfer control to an image unless its manifest identity, target, version, bounds, policy, and cryptographic digest have passed device-side validation.
- Keep PicoCalc peripheral GPIO and all external transmitters inactive until the board schematic revision and pin map are verified.
- Default new attachments to unpowered or receive-only operation. Active transmission, emulation, target power, and USB input automation require explicit local authorization and applicable engagement scope.
- Keep recovery paths bounded and available when an OS image is absent, malformed, corrupt, or repeatedly fails to boot.

### Security and ethical-use boundaries

- Design capabilities strictly for authorized security assessment, defensive research, education, and hardware experimentation.
- Treat SD content, network traffic, captures, update packages, attachment descriptors, and remote instructions as untrusted input.
- Enforce bounds, timeouts, capability checks, and engagement scope below application-level tools.
- Never weaken validation, authorization, audit, or transmit interlocks merely to simplify a feature demonstration.
- Do not store credentials, API keys, private keys, captures, or other secrets in source control or plaintext logs.

### Roadmap and documentation

- Implement work in the numbered slices in `PicoPen/docs/ROADMAP.md`; do not skip ahead without explicit user direction.
- Update the roadmap checklist and relevant design documentation in the same change that completes a slice or materially changes a contract.
- Record acceptance criteria and test evidence in the roadmap or directly linked documentation.
- Architecture, flash-layout, image-format, signing-policy, and hardware-pin decisions require user review before they are treated as final.

### Git practices

- Keep commits scoped to one completed slice or one clearly described corrective change.
- Commit only after builds and applicable tests pass, unless the user explicitly requests a work-in-progress checkpoint.
- Do not commit generated build outputs, local dependencies, captures, keys, credentials, or editor state.
- Use the repository-local author identity already configured by the user. Do not change global Git identity settings.
