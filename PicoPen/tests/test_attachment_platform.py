import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class AttachmentPlatformTests(unittest.TestCase):
    def test_registry_is_versioned_bounded_and_transport_agnostic(self):
        header = (ROOT / "services" / "attachment" / "include" / "picopen" /
                  "attachment.h").read_text(encoding="utf-8")
        source = (ROOT / "services" / "attachment" /
                  "attachment.c").read_text(encoding="utf-8")
        self.assertIn("PICOPEN_ATTACHMENT_ABI_VERSION 1u", header)
        self.assertIn("PICOPEN_ATTACHMENT_CAPACITY 8u", header)
        for transport in ("I2C", "SPI", "UART", "USB", "GPIO", "PIO",
                          "MOCK"):
            self.assertIn("PICOPEN_ATTACHMENT_TRANSPORT_" + transport, header)
        self.assertIn("picopen_attachment_has_provider", source)
        self.assertIn("registry.truncated = true", source)

    def test_active_and_observe_capabilities_are_separate(self):
        header = (ROOT / "services" / "attachment" / "include" / "picopen" /
                  "attachment.h").read_text(encoding="utf-8")
        for pair in (("NFC_READ", "NFC_EMULATE"),
                     ("SUBGHZ_RECEIVE", "SUBGHZ_TRANSMIT"),
                     ("IR_RECEIVE", "IR_TRANSMIT")):
            self.assertIn("PICOPEN_PROVIDER_" + pair[0], header)
            self.assertIn("PICOPEN_PROVIDER_" + pair[1], header)

    def test_mock_provider_has_no_hardware_io(self):
        main = (ROOT / "os" / "src" / "main.c").read_text(encoding="utf-8")
        attachment = (ROOT / "services" / "attachment" /
                      "attachment.c").read_text(encoding="utf-8")
        self.assertIn('"mock.pn532"', main)
        self.assertIn("PICOPEN_ATTACHMENT_DISABLED_POLICY", main)
        self.assertIn("PICOPEN_ATTACHMENT_TRANSPORT_MOCK", main)
        for forbidden in ("gpio_init", "gpio_put", "i2c_init", "spi_init",
                          "uart_init", "pio_add_program"):
            self.assertNotIn(forbidden, attachment)

    def test_apps_can_require_runtime_providers(self):
        header = (ROOT / "services" / "apps" / "include" / "picopen" /
                  "apps.h").read_text(encoding="utf-8")
        source = (ROOT / "services" / "apps" /
                  "apps.c").read_text(encoding="utf-8")
        gui = (ROOT / "services" / "gui" / "gui.c").read_text(
            encoding="utf-8")
        self.assertIn("required_provider", header)
        self.assertIn("picopen_attachment_has_provider", source)
        self.assertIn("picopen_app_available", gui)
        self.assertIn('" MISSING"', gui)


if __name__ == "__main__":
    unittest.main()
