import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tools" / "dependencies.json"
LOCK = ROOT / "tools" / "dependencies.lock"

ALLOWED_LICENSES = {"BSD-3-Clause", "MIT", "FatFs"}
ALLOWED_STATUS = {"incorporated", "approved"}


class DependencyManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.components = cls.manifest["components"]

    def test_schema_and_unique_names(self):
        self.assertEqual(self.manifest["schema_version"], 1)
        names = [component["name"] for component in self.components]
        self.assertEqual(len(names), len(set(names)))

    def test_components_have_approved_license_and_provenance(self):
        for component in self.components:
            with self.subTest(component=component["name"]):
                self.assertIn(component["status"], ALLOWED_STATUS)
                self.assertIn(component["license"], ALLOWED_LICENSES)
                self.assertTrue(component["source"].startswith(("https://", "http://")))
                self.assertTrue(component["pin"])
                self.assertTrue(component["purpose"])

    def test_incorporated_components_have_concrete_pins(self):
        for component in self.components:
            if component["status"] != "incorporated":
                continue
            with self.subTest(component=component["name"]):
                self.assertNotEqual(component["pin"], "pending")

    def test_pico_sdk_pin_matches_build_lock(self):
        lock_text = LOCK.read_text(encoding="utf-8")
        pico_sdk = next(
            component for component in self.components
            if component["name"] == "Raspberry Pi Pico SDK"
        )
        self.assertIn(f"PICO_SDK_REPOSITORY={pico_sdk['source']}", lock_text)
        self.assertIn(f"PICO_SDK_VERSION={pico_sdk['pin']}", lock_text)

    def test_fatfs_is_compiled_read_only(self):
        config = (ROOT / "third_party" / "fatfs" / "ffconf.h").read_text(
            encoding="utf-8"
        )
        required = {
            "FF_FS_READONLY": "1",
            "FF_USE_MKFS": "0",
            "FF_USE_CHMOD": "0",
            "FF_USE_LABEL": "0",
            "FF_USE_EXPAND": "0",
            "FF_USE_LFN": "0",
        }
        for name, value in required.items():
            with self.subTest(option=name):
                self.assertRegex(config, rf"#define\s+{name}\s+{value}\b")

    def test_fatfs_pin_matches_build_lock(self):
        lock_text = LOCK.read_text(encoding="utf-8")
        fatfs = next(
            component for component in self.components
            if component["name"] == "FatFs"
        )
        self.assertIn(f"FATFS_SOURCE_REPOSITORY={fatfs['source']}", lock_text)
        self.assertIn(f"FATFS_SOURCE_COMMIT={fatfs['pin']}", lock_text)


if __name__ == "__main__":
    unittest.main()
