# Apps and owner-mode baseline test

## Launcher and modes

1. Flash the combined slot image and confirm the former Workbench tile is Apps.
2. Open Apps and verify nine built-in entries are listed. Navigation should move
   once per key press and repeat at a controlled rate while an arrow is held.
   Open Device Inventory and verify it immediately shows the current registered
   device states; it must not open the retired Workbench scan/progress screen.
3. Open System > Security. Verify the authorization editor has reference,
   optional Limit, duration, activate/end, and Mode fields with no port field.
4. Select Mode repeatedly and verify Owner, Guarded, and Developer cycle and
   survive a reboot. Owner is the default after a settings-format migration.

## App task settings

1. In Owner mode with no active boundary, open HTTP Inspector. Enter an owned
   lab target, accept port 80 or adjust it, review the task, and run it.
   The result screen must show state, address, service, detail, and result code.
   If startup fails, the configuration screen must show the failure instead.
2. Repeat with SSH Banner and TLS Inspector; defaults should be 22 and 443.
   TLS remains deliberately unavailable pending its reviewed client setup.
3. Activate a single-host or CIDR boundary and verify an outside target is
   denied even in Owner mode.
4. Switch to Guarded, end the session, and verify active network tasks are
   denied until a session is activated. Limit may remain blank; setting it adds
   a host/CIDR restriction to every app target.
5. Network Discovery must show passive local interface/AP information only. It
   must not treat a blank field as permission for an active network scan.

## SD packages and reports

1. If possible, create `/PicoPen/apps` on the FAT card and place a harmless file
   ending in `.ppapp` there. Reboot or rescan and confirm it appears with `SD`.
2. Opening the entry must state that validation/runtime support is pending and
   must not execute the file.
3. Open Session Reports and confirm it shows volatile result/audit counts and
   clearly states export is unavailable while SD remains read-only.
