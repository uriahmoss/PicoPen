import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class HostWorkflowTests(unittest.TestCase):
    def test_inventory_is_bounded_and_collects_known_sources(self):
        header = (ROOT / "services" / "hosts" / "include" / "picopen" /
                  "hosts.h").read_text(encoding="utf-8")
        source = (ROOT / "services" / "hosts" /
                  "hosts.c").read_text(encoding="utf-8")
        self.assertIn("PICOPEN_HOST_CAPACITY 16u", header)
        self.assertIn("PICOPEN_HOST_SERVICE_CAPACITY 6u", header)
        for field in ("wifi->ipv4", "wifi->gateway", "wifi->dns"):
            self.assertIn(field, source)
        self.assertIn("picopen_hosts_observe_recon", source)
        self.assertIn("inventory.truncated = true", source)

    def test_host_actions_use_app_kinds_not_catalog_positions(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8")
        self.assertIn("GUI_HOSTS", gui)
        self.assertIn("GUI_HOST_ACTIONS", gui)
        self.assertIn("app_index_for_kind", gui)
        self.assertIn("PICOPEN_APP_HTTP_INSPECTOR", gui)
        self.assertIn("PICOPEN_APP_SSH_BANNER", gui)
        self.assertIn("PICOPEN_APP_TLS_INSPECTOR", gui)
        self.assertIn("task_return_screen=GUI_HOST_ACTIONS", gui)
        recon_escape = gui[gui.index("if (screen == GUI_RECON) {"):
                           gui.index("if (screen == GUI_EVIDENCE) {")]
        self.assertIn("screen = task_return_screen", recon_escape)
        self.assertNotIn("screen = GUI_APPS", recon_escape)

    def test_recent_results_have_details_and_reuse(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8")
        self.assertIn("GUI_RECON_HISTORY_DETAIL", gui)
        self.assertIn("render_recon_history_detail", gui)
        self.assertIn("ENTER USE HOST", gui)
        self.assertIn("selected_result.address", gui)


if __name__ == "__main__":
    unittest.main()
