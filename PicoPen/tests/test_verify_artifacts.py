import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).parents[1] / "tools"))
import verify_artifacts as artifacts


def uf2_block(address, number=0, count=1,
              family=artifacts.RP2350_ARM_S_FAMILY_ID):
    block = bytearray(artifacts.UF2_SIZE)
    struct.pack_into("<IIIIIIII", block, 0,
                     artifacts.UF2_MAGIC_START0,
                     artifacts.UF2_MAGIC_START1,
                     artifacts.UF2_FLAG_FAMILY_ID,
                     address, 256, number, count, family)
    struct.pack_into("<I", block, 508, artifacts.UF2_MAGIC_END)
    return bytes(block)


class ArtifactVerificationTests(unittest.TestCase):
    def verify(self, blob, region=(0x10000000, 0x10040000)):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "test.uf2"
            path.write_bytes(blob)
            return artifacts.verify_uf2(path, region)

    def test_accepts_bounded_secure_uf2(self):
        result = self.verify(uf2_block(0x10000000))
        self.assertEqual(result["start"], 0x10000000)
        self.assertEqual(result["end"], 0x10000100)

    def test_rejects_wrong_family_and_out_of_bounds_data(self):
        with self.assertRaises(ValueError):
            self.verify(uf2_block(0x10000000, family=0))
        with self.assertRaises(ValueError):
            self.verify(uf2_block(0x10040000))

    def test_rejects_missing_and_overlapping_blocks(self):
        with self.assertRaises(ValueError):
            self.verify(uf2_block(0x10000000, number=0, count=2))
        duplicate = (uf2_block(0x10000000, 0, 2) +
                     uf2_block(0x10000080, 1, 2))
        with self.assertRaises(ValueError):
            self.verify(duplicate)


if __name__ == "__main__":
    unittest.main()
