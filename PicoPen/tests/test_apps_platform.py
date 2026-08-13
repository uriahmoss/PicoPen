import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class AppsPlatformTests(unittest.TestCase):
    def test_apps_catalog_has_builtins_and_bounded_sd_discovery(self):
        header = (ROOT / "services" / "apps" / "include" / "picopen" /
                  "apps.h").read_text(encoding="utf-8")
        source = (ROOT / "services" / "apps" / "apps.c").read_text(
            encoding="utf-8")
        self.assertIn("PICOPEN_APP_CAPACITY 16u", header)
        self.assertIn("PICOPEN_APP_BUILTIN_COUNT 9u", header)
        self.assertIn('"/PicoPen/apps"', source)
        self.assertIn('".ppapp"', source)
        self.assertIn("app->compatible = false", source)

    def test_scope_boundary_is_port_independent(self):
        policy = (ROOT / "services" / "policy" /
                  "engagement.c").read_text(encoding="utf-8")
        header = (ROOT / "services" / "policy" / "include" / "picopen" /
                  "engagement.h").read_text(encoding="utf-8")
        self.assertIn("picopen_engagement_session_activate_boundary", header)
        self.assertIn("picopen_engagement_session_activate_optional_boundary",
                      header)
        self.assertIn("boundary_configured", header)
        self.assertIn("if (!session.boundary_configured) return true", policy)
        self.assertIn("(void)port", policy)
        self.assertNotIn("port_first;", header)

    def test_owner_mode_and_per_app_inputs_are_persistent(self):
        preferences = (ROOT / "services" / "settings" / "preferences.c").read_text(
            encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(encoding="utf-8")
        self.assertIn("PREFERENCES_VERSION 2u", preferences)
        self.assertIn("picopen_preferences_set_security_mode", preferences)
        self.assertIn("GUI_APP_CONFIG", gui)
        self.assertIn("task_target", gui)
        self.assertIn("task_port", gui)
        self.assertIn("security_mode == PICOPEN_SECURITY_GUARDED", gui)
        self.assertIn("task_security.engagement_active = true", gui)
        self.assertIn("if (started)", gui)
        self.assertIn("START FAILED: CHECK WIFI / RETRY", gui)
        self.assertIn('"<OPTIONAL>"', gui)

    def test_device_inventory_does_not_launch_legacy_workbench(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8")
        apps_handler = gui[gui.index("if (screen == GUI_APPS) {"):
                           gui.index("if(screen==GUI_APP_CONFIG)")]
        self.assertIn("PICOPEN_APP_DEVICE_INVENTORY", apps_handler)
        self.assertIn("screen=GUI_APP_DEVICES", apps_handler)
        self.assertNotIn("picopen_workbench_start", apps_handler)

    def test_network_results_repaint_during_and_after_async_tasks(self):
        recon = (ROOT / "services" / "recon" / "recon.c").read_text(
            encoding="utf-8")
        main = (ROOT / "os" / "src" / "main.c").read_text(
            encoding="utf-8")
        self.assertIn("RECON_PROGRESS_INTERVAL_MS", recon)
        self.assertIn("snapshot_dirty = true", recon)
        self.assertIn("now_ms >= next_progress_ms", recon)
        self.assertIn("snapshot.state >= PICOPEN_RECON_COMPLETE", main)


if __name__ == "__main__":
    unittest.main()
