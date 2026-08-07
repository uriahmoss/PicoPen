import binascii
import hashlib
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).parents[1] / "tools"))
import package_image as image


class PackageImageTests(unittest.TestCase):
    def setUp(self):
        self.payload = bytes(range(256)) * 4
        self.packed = image.build_image(
            self.payload, image.PAYLOAD_ADDRESS + 4, bytes(32), (0, 0, 1), 1)

    def test_manifest_and_payload(self):
        image.validate_image(self.packed)
        self.assertEqual(len(self.packed), image.MANIFEST_SIZE + len(self.payload))
        self.assertEqual(self.packed[:8], image.MAGIC)
        self.assertEqual(self.packed[image.MANIFEST_SIZE:], self.payload)
        self.assertTrue(all(value == 0xFF for value in
                            self.packed[image.HEADER_SIZE:image.MANIFEST_SIZE]))

    def test_crc_and_digest(self):
        header = struct.unpack(image.FORMAT, self.packed[:image.HEADER_SIZE])
        self.assertEqual(header[-1], binascii.crc32(
            self.packed[:image.HEADER_SIZE - 4]) & 0xFFFFFFFF)
        self.assertEqual(header[18], hashlib.sha256(self.payload).digest())

    def test_rejects_empty_and_oversized_payloads(self):
        with self.assertRaises(ValueError):
            image.build_image(b"", image.PAYLOAD_ADDRESS, bytes(32), (0, 0, 1), 1)
        with self.assertRaises(ValueError):
            image.build_image(bytes(image.MAX_PAYLOAD_SIZE + 1),
                              image.PAYLOAD_ADDRESS, bytes(32), (0, 0, 1), 1)

    def test_rejects_entry_outside_payload(self):
        with self.assertRaises(ValueError):
            image.build_image(self.payload, image.PAYLOAD_ADDRESS - 1,
                              bytes(32), (0, 0, 1), 1)

    def test_rejects_corrupted_payload(self):
        corrupted = bytearray(self.packed)
        corrupted[-1] ^= 1
        with self.assertRaisesRegex(ValueError, "digest"):
            image.validate_image(bytes(corrupted))

    def test_rejects_oversized_header_with_valid_crc(self):
        corrupted = bytearray(self.packed)
        struct.pack_into("<I", corrupted, 20, image.MAX_PAYLOAD_SIZE + 1)
        struct.pack_into("<I", corrupted, image.HEADER_SIZE - 4,
                         binascii.crc32(corrupted[:image.HEADER_SIZE - 4]) &
                         0xFFFFFFFF)
        with self.assertRaisesRegex(ValueError, "bounds"):
            image.validate_image(bytes(corrupted))


if __name__ == "__main__":
    unittest.main()
