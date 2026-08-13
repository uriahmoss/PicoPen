import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ScopedReconEvidenceTests(unittest.TestCase):
    def test_recon_is_bounded_and_rechecks_shared_scope(self):
        recon = (ROOT / "services" / "recon" / "recon.c").read_text(
            encoding="utf-8"
        )
        header = (ROOT / "services" / "recon" / "include" / "picopen" /
                  "recon.h").read_text(encoding="utf-8")
        self.assertIn("RECON_TIMEOUT_MS UINT64_C(7000)", recon)
        self.assertIn("RECON_RATE_MS UINT64_C(1000)", recon)
        self.assertIn("locally_confirmed", recon)
        self.assertIn("picopen_engagement_session_allows_hostname", recon)
        self.assertIn("picopen_engagement_session_allows_ipv4", recon)
        self.assertIn("target_allowed(current.target", recon)
        for forbidden in ("tcp_bind(", "tcp_listen(", "tcp_accept("):
            self.assertNotIn(forbidden, recon)

    def test_scope_has_target_and_port_constraints(self):
        header = (ROOT / "services" / "policy" / "include" / "picopen" /
                  "engagement.h").read_text(encoding="utf-8")
        policy = (ROOT / "services" / "policy" /
                  "engagement.c").read_text(encoding="utf-8")
        self.assertIn("picopen_engagement_session_activate_scoped", header)
        self.assertIn("port_first", header)
        self.assertIn("port_last", header)
        self.assertIn("prefix>32u", policy)

    def test_evidence_is_read_only_and_size_bounded(self):
        evidence = (ROOT / "services" / "evidence" /
                    "evidence.c").read_text(encoding="utf-8")
        self.assertIn("EVIDENCE_MAX_SIZE UINT32_C(262144)", evidence)
        self.assertIn("picopen_storage_read_file", evidence)
        self.assertIn("mbedtls_sha256_update", evidence)
        self.assertIn("PICOPEN_CAPTURE_PCAPNG", evidence)
        self.assertIn("string_preview", evidence)
        self.assertIn("inspect_ethernet", evidence)
        self.assertIn("picopen_evidence_digest_matches", evidence)
        for write_api in ("picopen_storage_write", "f_write(", "flash_range_program"):
            self.assertNotIn(write_api, evidence)

    def test_workbench_results_can_be_dismissed_without_leaving_menu(self):
        workbench = (ROOT / "services" / "workbench" /
                     "workbench.c").read_text(encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("picopen_workbench_dismiss", workbench)
        self.assertIn("selection = workbench_selection", gui)
        recon_escape = gui.index("if (screen == GUI_RECON)")
        global_parent = gui.index("screen = parent_screen(screen)")
        self.assertLess(recon_escape, global_parent)

    def test_successful_workbench_navigation_is_rendered(self):
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        workbench_handler = gui[gui.index("if (screen == GUI_APPS) {"):
                                gui.index("if((screen==GUI_RECON)")]
        self.assertIn("const size_t previous = selection", workbench_handler)
        self.assertIn("if (selection != previous)", workbench_handler)
        self.assertIn("render_workbench();", workbench_handler)
        self.assertIn("else if (screen == GUI_APPS)", gui)

    def test_network_inspector_requires_confirmation_and_stays_bounded(self):
        recon = (ROOT / "services" / "recon" / "recon.c").read_text(
            encoding="utf-8"
        )
        header = (ROOT / "services" / "recon" / "include" / "picopen" /
                  "recon.h").read_text(encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("GUI_RECON_CONFIRM", gui)
        self.assertIn("ONE REQUEST, 7 SECOND DEADLINE", gui)
        self.assertIn("HEAD / HTTP/1.1", recon)
        self.assertIn("PICOPEN_RECON_SSH_BANNER", header)
        self.assertIn("RECON_RESPONSE_LIMIT 512u", recon)
        self.assertIn("PICOPEN_RECON_HISTORY_CAPACITY", recon)
        self.assertNotIn("Authorization:", recon)
        self.assertNotIn("tcp_bind(", recon)
        self.assertNotIn("tcp_listen(", recon)

    def test_tls_inspection_fails_closed_until_reviewed(self):
        recon = (ROOT / "services" / "recon" / "recon.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("PICOPEN_RECON_UNAVAILABLE", recon)
        self.assertIn("TLS CLIENT DISABLED PENDING RAM REVIEW", recon)

    def test_held_key_repeat_excludes_enter_and_escape(self):
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn("event.state == PICOPEN_KEY_HELD && navigation_key", main)
        self.assertIn("pressed ? 300u : 140u", main)


if __name__ == "__main__":
    unittest.main()
