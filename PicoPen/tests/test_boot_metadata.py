import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).parents[1] / "tools"))
import boot_metadata as metadata


class BootMetadataTests(unittest.TestCase):
    def test_round_trip_pending_attempt(self):
        record = metadata.encode(7, metadata.FLAG_PENDING, 2, 0x504F5345)
        decoded = metadata.decode(record)
        self.assertEqual(decoded["generation"], 7)
        self.assertEqual(decoded["attempts"], 2)
        self.assertEqual(decoded["last_failure"], 0x504F5345)

    def test_rejects_corruption_and_invalid_attempt_count(self):
        record = bytearray(metadata.encode(1, metadata.FLAG_CONFIRMED, 0))
        record[20] ^= 1
        with self.assertRaises(ValueError):
            metadata.decode(record)
        with self.assertRaises(ValueError):
            metadata.encode(1, metadata.FLAG_PENDING, 4)

    def test_selects_newest_valid_copy(self):
        older = metadata.encode(10, metadata.FLAG_PENDING, 1)
        newer = metadata.encode(11, metadata.FLAG_PENDING, 2)
        self.assertEqual(metadata.select(older, newer)["attempts"], 2)

    def test_falls_back_after_interrupted_write(self):
        valid = metadata.encode(20, metadata.FLAG_PENDING, 2)
        incomplete = bytes(256)
        self.assertEqual(metadata.select(valid, incomplete)["generation"], 20)

    def test_generation_selection_wraps(self):
        before_wrap = metadata.encode(0xFFFFFFFF, metadata.FLAG_PENDING, 1)
        after_wrap = metadata.encode(0, metadata.FLAG_CONFIRMED, 0)
        self.assertEqual(metadata.select(before_wrap, after_wrap)["generation"], 0)


if __name__ == "__main__":
    unittest.main()
