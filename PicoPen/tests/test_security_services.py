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

    def test_removable_file_reads_are_bounded_and_path_validated(self):
        header = (ROOT / "services" / "storage" / "include" / "picopen" /
                  "storage.h").read_text(encoding="utf-8")
        implementation = (ROOT / "services" / "storage" /
                          "storage.c").read_text(encoding="utf-8")
        self.assertRegex(header, r"PICOPEN_STORAGE_READ_LIMIT\s+256u")
        for forbidden in ("character == '\\\\'", "character == ':'"):
            self.assertIn(forbidden, implementation)
        self.assertRegex(header, r"PICOPEN_STORAGE_MAX_DEPTH\s+4u")
        self.assertIn("component[1] == '.'", implementation)
        self.assertIn("FA_READ", implementation)

    def test_versioned_storage_tracks_media_and_never_exposes_write(self):
        header = (ROOT / "services" / "storage" / "include" / "picopen" /
                  "storage.h").read_text(encoding="utf-8")
        implementation = (ROOT / "services" / "storage" /
                          "storage.c").read_text(encoding="utf-8")
        self.assertIn("PICOPEN_STORAGE_ABI_VERSION 1u", header)
        self.assertIn("media_generation", header)
        self.assertIn("PICOPEN_STORAGE_MEDIA_READY_READ_ONLY", header)
        self.assertIn("picopen_storage_safe_remove", implementation)
        for forbidden in ("FA_WRITE", "f_unlink", "f_mkdir", "f_rename"):
            self.assertNotIn(forbidden, implementation)

    def test_file_gui_supports_bounded_metadata_text_and_hex_views(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("entry->size", gui)
        self.assertIn("HEX (FIRST 96 BYTES)", gui)
        self.assertIn("text_bytes * 100u", gui)
        self.assertIn("picopen_storage_list_directory", gui)

    def test_passive_workbench_job_is_bounded_and_does_not_touch_hardware(self):
        header = (ROOT / "services" / "workbench" / "include" / "picopen" /
                  "workbench.h").read_text(encoding="utf-8")
        implementation = (ROOT / "services" / "workbench" /
                          "workbench.c").read_text(encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("PICOPEN_WORKBENCH_ABI_VERSION 1u", header)
        self.assertIn("PICOPEN_WORKBENCH_ITEM_CAPACITY 7u", header)
        self.assertIn("PICOPEN_WORKBENCH_RUNNING", header)
        self.assertIn("picopen_workbench_cancel", implementation)
        self.assertIn("PICOPEN_WORKBENCH_STEP_MS", implementation)
        for forbidden in ("hardware/gpio.h", "hardware/i2c.h",
                          "hardware/spi.h", "gpio_put(", "i2c_write",
                          "spi_write"):
            self.assertNotIn(forbidden, implementation)
        self.assertIn("CONFIG/POLICY INVENTORY ONLY", gui)
        self.assertIn("NO BUS TRAFFIC OR PIN CHANGES", gui)
        self.assertIn('picopen_audit_record("workbench.start"', gui)

    def test_engagement_editor_is_bounded_local_and_does_not_grant_capabilities(self):
        header = (ROOT / "services" / "policy" / "include" / "picopen" /
                  "engagement.h").read_text(encoding="utf-8")
        policy = (ROOT / "services" / "policy" / "engagement.c").read_text(
            encoding="utf-8"
        )
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn("PICOPEN_ENGAGEMENT_REFERENCE_SIZE 24u", header)
        self.assertIn("PICOPEN_ENGAGEMENT_MAX_DURATION_MS", header)
        self.assertIn("local_confirmation", policy)
        self.assertIn("valid_reference", policy)
        self.assertIn("SCOPE DOES NOT GRANT CAPABILITIES", gui)
        self.assertIn("picopen_engagement_session_activate", gui)
        self.assertIn('picopen_audit_record(active ? "scope.end"', gui)
        self.assertIn("shell_state.security.engagement_active", main)
        grants = re.search(r"shell_security\.grants\s*=\s*(?P<body>.*?);",
                           main, re.DOTALL).group("body")
        for forbidden in ("PICOPEN_CAP_GPIO_DRIVE", "PICOPEN_CAP_RADIO_TRANSMIT",
                          "PICOPEN_CAP_TARGET_POWER", "PICOPEN_CAP_REMOTE_CONTROL"):
            self.assertNotIn(forbidden, grants)

    def test_scope_indicator_is_owned_by_each_theme_renderer(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        synthwave = (ROOT / "services" / "appearance" /
                     "synthwave_renderer.c").read_text(encoding="utf-8")
        crayon = (ROOT / "services" / "appearance" /
                  "crayon_renderer.c").read_text(encoding="utf-8")
        self.assertRegex(gui, r"renderer_home\(labels, GUI_HOME_ITEMS, selection,\s+scope_active\)")
        self.assertNotIn("picopen_display_fill_rect(240u, 4u, 76u, 14u", gui)
        self.assertIn("bool scope_active", synthwave)
        self.assertIn('scope_active ? "SCOPE ON" : "SCOPE OFF"', synthwave)
        self.assertIn("bool scope_active", crayon)
        self.assertIn('scope_active ? "SCOPE ON" : "SCOPE OFF"', crayon)

    def test_root_file_menu_remembers_its_selection(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("static size_t files_selection", gui)
        self.assertIn("files_selection = selection", gui)
        self.assertIn("files_selection < file_listing.count", gui)

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

    def test_wifi_ui_uses_service_and_cannot_connect_or_listen(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        wifi = (ROOT / "services" / "network" / "wifi.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("NO LISTENERS", gui)
        self.assertIn("picopen_wifi_vault_save", gui)
        self.assertIn("picopen_wifi_vault_load", gui)
        self.assertIn("PICOPEN_CAP_NETWORK_CONNECT, true", gui)
        self.assertNotIn("cyw43_arch_enable_sta_mode", gui)
        self.assertNotIn("cyw43_arch_wifi_connect", gui)
        self.assertIn("locally_confirmed", wifi)
        self.assertIn("cyw43_arch_enable_sta_mode", wifi)
        for forbidden in ("tcp_", "udp_", "http", "listen(", "accept("):
            self.assertNotIn(forbidden, wifi.lower())
        self.assertIn(".scan_type = 1", wifi)
        self.assertIn("scrub(password)", wifi)
        self.assertIn("15000u", wifi)

    def test_wifi_vault_is_authenticated_bounded_and_not_unattended(self):
        vault = (ROOT / "services" / "vault" / "wifi_vault.c").read_text(
            encoding="utf-8"
        )
        storage = (ROOT / "services" / "settings" / "internal_fs.c").read_text(
            encoding="utf-8"
        )
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn("mbedtls_gcm_auth_decrypt", vault)
        self.assertIn("mbedtls_pkcs5_pbkdf2_hmac_ext", vault)
        self.assertIn("pico_get_unique_board_id", vault)
        self.assertIn("VAULT_MAX_FAILURES 5u", vault)
        self.assertIn("VAULT_LOCK_MS 30000u", vault)
        self.assertIn("mbedtls_platform_zeroize", vault)
        self.assertNotRegex(vault, r"\bprintf\(")
        self.assertIn("PICOPEN_PERSISTENT_OFFSET", storage)
        self.assertIn("save_and_disable_interrupts", storage)
        self.assertIn("locally_confirmed", storage)
        self.assertIn("picopen_internal_fs_init();", main)
        self.assertNotIn("picopen_wifi_vault_load", main)

    def test_wifi_starts_off_and_network_connect_requires_local_confirmation(self):
        wifi = (ROOT / "services" / "network" / "wifi.c").read_text(
            encoding="utf-8"
        )
        capability = (ROOT / "kernel" / "capability.c").read_text(
            encoding="utf-8"
        )
        cmake = (ROOT / "os" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn(".state = PICOPEN_WIFI_OFF", wifi)
        self.assertIn("cyw43_arch_disable_sta_mode", wifi)
        self.assertIn("PICOPEN_CAP_NETWORK_CONNECT", capability)
        self.assertIn("pico_cyw43_arch_lwip_poll", cmake)
        self.assertIn("LWIP_SOCKET 0", (ROOT / "config" / "lwipopts.h").read_text(encoding="utf-8"))
        self.assertIn("LWIP_TCP 0", (ROOT / "config" / "lwipopts.h").read_text(encoding="utf-8"))

    def test_network_preferences_and_recovery_fast_track(self):
        wifi = (ROOT / "services" / "network" / "wifi.c").read_text(encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(encoding="utf-8")
        preferences = (ROOT / "services" / "settings" / "preferences.c").read_text(encoding="utf-8")
        recovery = (ROOT / "services" / "recovery" / "recovery.c").read_text(encoding="utf-8")
        self.assertIn("netif_default", wifi)
        self.assertIn("dns_getserver", wifi)
        self.assertIn("picopen_wifi_select_ap", gui)
        self.assertIn("PREFERENCES_VERSION 1u", preferences)
        self.assertIn("offsetof(picopen_preferences_t, checksum)", preferences)
        self.assertIn("PICOPEN_BOOT_ATTEMPT_OS_ENTERED", recovery)
        self.assertIn("PICOPEN_GUI_STORAGE_SAFE_REMOVE", gui)
        self.assertIn("PICOPEN_GUI_STORAGE_RESCAN", gui)

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

    def test_skin_service_keeps_synthwave_as_factory_default(self):
        skin = (ROOT / "services" / "appearance" / "skin.c").read_text(
            encoding="utf-8"
        )
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("current_skin = PICOPEN_SKIN_SYNTHWAVE", skin)
        for name in ("SYNTHWAVE", "CRAYON", "HIGH CONTRAST", "MINIMAL DARK"):
            self.assertIn(name, skin)
        self.assertIn("textured_focus", gui)
        self.assertIn("SESSION DEFAULT", gui)

    def test_skin_schema_is_data_only(self):
        header = (ROOT / "services" / "appearance" / "include" / "picopen" /
                  "skin.h").read_text(encoding="utf-8")
        self.assertNotIn("callback", header.lower())
        self.assertNotIn("script", header.lower())
        self.assertNotIn("void *", header)

    def test_complex_skin_renderer_is_bounded(self):
        terminal = (ROOT / "drivers" / "terminal" / "terminal.c").read_text(
            encoding="utf-8"
        )
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("PICOPEN_TEXT_CRAYON", terminal)
        self.assertIn("PICOPEN_TEXT_NEON", terminal)
        self.assertIn("scale > 4u", terminal)
        self.assertIn("skin->textured_focus", gui)
        self.assertIn("PICOPEN_SKIN_STYLE_NEON", gui)
        self.assertIn("PICOPEN_SKIN_STYLE_CRAYON", gui)
        self.assertIn("for (uint16_t row = 0u; (row < 20u)", gui)
        self.assertIn("strcmp(lines[row], rendered_lines[row])", gui)
        self.assertIn("draw_page_line", gui)
        self.assertIn("crayon ? 32u : 74u", gui)

    def test_synthwave_and_crayon_own_independent_renderers(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        crayon = (ROOT / "services" / "appearance" /
                   "crayon_renderer.c").read_text(encoding="utf-8")
        synthwave = (ROOT / "services" / "appearance" /
                     "synthwave_renderer.c").read_text(encoding="utf-8")
        self.assertIn("picopen_crayon_renderer_home", gui)
        self.assertIn("picopen_synthwave_renderer_home", gui)
        self.assertIn("crayon_screen_asset.h", crayon)
        self.assertIn("picopen_display_blit_indexed8", crayon)
        self.assertIn("draw_home_focus", crayon)
        self.assertIn("picopen_crayon_focus_pixels", crayon)
        self.assertIn("picopen_crayon_selected_pixels", crayon)
        self.assertIn("draw_page_entry", crayon)
        self.assertIn("draw_crayon_text_transparent", crayon)
        self.assertNotIn("graphical_tile", crayon)
        self.assertIn("grid", synthwave)
        self.assertIn("tile", synthwave)
        self.assertNotIn("my security sketchbook", synthwave)

    def test_bitmap_compositor_is_bounded(self):
        header = (ROOT / "drivers" / "display" / "include" / "picopen" /
                  "display.h").read_text(encoding="utf-8")
        display = (ROOT / "drivers" / "display" /
                   "display.c").read_text(encoding="utf-8")
        asset = (ROOT / "services" / "appearance" / "include" / "picopen" /
                 "crayon_screen_asset.h").read_text(encoding="utf-8")
        self.assertIn("picopen_display_blit_indexed8", header)
        self.assertIn("picopen_display_blit_indexed8_keyed", header)
        self.assertIn("palette_size > 256u", display)
        self.assertIn("stride < width", display)
        self.assertIn("PICOPEN_CRAYON_SCREEN_WIDTH 320u", asset)
        self.assertIn("PICOPEN_CRAYON_SCREEN_HEIGHT 320u", asset)
        self.assertIn("PICOPEN_CRAYON_SCREEN_COLORS 64u", asset)


if __name__ == "__main__":
    unittest.main()
