# Passive Workbench Fast-Track Test

This image adds a bounded configuration and policy inventory job. The job does
not initialize, sample, probe, or change GPIO, ADC, I2C, SPI, UART, radio, or
attachment hardware. It reports verified board ownership and the locked or
unverified state of expansion interfaces.

## Flash

Use the verified PicoPen bootloader and flash
`build/pico2w-debug/os/picopen_os_slot.uf2` with the established OS-slot
procedure. No SD test files or attachments are required.

## Acceptance checks

1. Boot to Home and open **Workbench**.
2. Confirm the initial job is `IDLE` and the screen says
   `CONFIG/POLICY INVENTORY ONLY` and `NO BUS TRAFFIC OR PIN CHANGES`.
3. Press Enter. Confirm the job becomes `RUNNING`, progress advances, and the
   fixed inventory fills in without keyboard input becoming unresponsive.
4. Confirm the finished job reaches `COMPLETE 100%` and reports:
   - I2C1 keyboard claimed when available;
   - SPI0 SD read-only when available;
   - SPI1 display claimed;
   - GPIO expansion disabled;
   - ADC and UART expansion unverified; and
   - attachments disabled.
5. Press Enter to start it again, then press Escape while it is running.
   Confirm the state becomes `CANCELLED`. Press Escape again to return Home.
6. Open **Audit**. Confirm a `workbench` lifecycle action is the latest record
   after starting, cancelling, or completing a job.
7. Re-enter Workbench and run the job repeatedly. Confirm job identifiers and
   progress remain bounded and the UI stays responsive.

This is not an electrical bus scan. Live receive-only tools will require a
reviewed expansion connector pin map, voltage rules, and sampling limits.
