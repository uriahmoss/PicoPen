import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class SecurityServiceBaselineTests(unittest.TestCase):
    def test_ipc_capability_is_owned_by_registered_service(self):
        header = (ROOT / "kernel" / "include" / "picopen" / "ipc.h").read_text(
            encoding="utf-8"
        )
        implementation = (ROOT / "kernel" / "ipc.c").read_text(encoding="utf-8")
        message = re.search(
            r"typedef struct picopen_ipc_message \{(?P<body>.*?)\}",
            header,
            re.DOTALL,
        ).group("body")
        service = re.search(
            r"typedef struct picopen_ipc_service \{(?P<body>.*?)\}",
            header,
            re.DOTALL,
        ).group("body")
        self.assertNotIn("required_capability", message)
        self.assertIn("required_capability", service)
        self.assertIn("service->required_capability", implementation)

    def test_removable_file_reads_are_bounded_and_root_only(self):
        header = (ROOT / "services" / "storage" / "include" / "picopen" /
                  "storage.h").read_text(encoding="utf-8")
        implementation = (ROOT / "services" / "storage" /
                          "storage.c").read_text(encoding="utf-8")
        self.assertRegex(header, r"PICOPEN_STORAGE_READ_LIMIT\s+256u")
        for forbidden in ("character == '/'", "character == '\\\\'",
                          "character == ':'"):
            self.assertIn(forbidden, implementation)
        self.assertIn("FA_READ", implementation)

    def test_shutdown_requires_explicit_local_confirmation(self):
        capability = (ROOT / "kernel" / "capability.c").read_text(
            encoding="utf-8"
        )
        shell = (ROOT / "services" / "shell" / "shell.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("PICOPEN_CAP_SYSTEM_SHUTDOWN", capability)
        self.assertIn('strcmp(command, "shutdown confirm")', shell)
        self.assertIn("PICOPEN_CAP_SYSTEM_SHUTDOWN, true", shell)

    def test_gui_shutdown_is_persistent_and_locally_confirmed(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("NO TIME LIMIT", gui)
        self.assertNotIn("shutdown_deadline", gui)
        self.assertIn("PICOPEN_CAP_SYSTEM_SHUTDOWN, true", gui)
        self.assertIn("picopen_keyboard_request_shutdown(6u)", gui)

    def test_wifi_update_ui_cannot_enable_networking(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("NOT CONFIGURED", gui)
        self.assertIn("WIFI REMAINS DISABLED BY POLICY", gui)
        self.assertNotIn("cyw43_arch_enable_sta_mode", gui)
        self.assertNotIn("cyw43_arch_wifi_connect", gui)

    def test_graphical_home_uses_bounded_direct_widgets(self):
        terminal = (ROOT / "drivers" / "terminal" / "terminal.c").read_text(
            encoding="utf-8"
        )
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("scale > 4u", terminal)
        self.assertIn("graphical_tile", gui)
        self.assertIn("GUI_COLOR_FOCUS", gui)
        self.assertIn("redraw_home_focus", gui)
        self.assertIn("SCOPE OFF", gui)
        self.assertIn("LOCKED", gui)

    def test_os_boot_does_not_require_usb_connection(self):
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        self.assertNotIn("PICOPEN_OS_USB_WAIT_MS", main)
        self.assertNotIn("PICOPEN_BOOT_ATTEMPT_USB_TIMEOUT", main)
        self.assertIn("boot-independent", main)

    def test_peripheral_recovery_is_bounded_and_updates_services(self):
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn("PICOPEN_CONTROLLER_READY_MS 5000u", main)
        self.assertIn("picopen_keyboard_health_check", main)
        self.assertIn("picopen_device_set_state", main)
        self.assertIn("picopen_shell_update_state", main)
        self.assertIn("picopen_gui_update_state", main)


if __name__ == "__main__":
    unittest.main()
