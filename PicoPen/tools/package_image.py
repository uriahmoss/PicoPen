#!/usr/bin/env python3
"""Create and inspect a deterministic PicoPen development image."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import pathlib
import struct

MAGIC = b"PICOPEN\0"
HEADER_SIZE = 0x100
MANIFEST_SIZE = 0x1000
MAX_PAYLOAD_SIZE = 0x1BF000
PAYLOAD_ADDRESS = 0x10051000
TARGET_PICO2_W = 0x50325700
FLAG_DEVELOPMENT = 1
FORMAT = "<8sHHIIIIIIHHHHQIHH16s32s64s32s48sI"


def elf_entry(path: pathlib.Path) -> int:
    data = path.read_bytes()[:52]
    if len(data) < 52 or data[:4] != b"\x7fELF":
        raise ValueError("input is not an ELF32 file")
    if data[4] != 1 or data[5] != 1:
        raise ValueError("only little-endian ELF32 is supported")
    return struct.unpack_from("<I", data, 24)[0]


def build_image(payload: bytes, entry_address: int, provenance: bytes,
                version: tuple[int, int, int], build_number: int) -> bytes:
    if not payload or len(payload) > MAX_PAYLOAD_SIZE:
        raise ValueError("payload size is outside the primary image slot")
    entry_offset = entry_address - PAYLOAD_ADDRESS
    if entry_offset < 0 or entry_offset >= len(payload):
        raise ValueError("ELF entry point is outside the packaged payload")

    values = (
        MAGIC, 1, HEADER_SIZE, FLAG_DEVELOPMENT, TARGET_PICO2_W,
        len(payload), MANIFEST_SIZE, entry_offset, 0,
        version[0], version[1], version[2], 0, build_number, 1,
        1, 0, bytes(16), hashlib.sha256(payload).digest(), bytes(64),
        provenance, bytes(48), 0,
    )
    header = bytearray(struct.pack(FORMAT, *values))
    if len(header) != HEADER_SIZE:
        raise AssertionError("image header layout drifted from 256 bytes")
    struct.pack_into("<I", header, HEADER_SIZE - 4,
                     binascii.crc32(header[:-4]) & 0xFFFFFFFF)
    return bytes(header) + bytes([0xFF]) * (MANIFEST_SIZE - HEADER_SIZE) + payload


def validate_image(blob: bytes) -> None:
    if len(blob) < MANIFEST_SIZE + 1:
        raise ValueError("image is truncated")
    header_bytes = blob[:HEADER_SIZE]
    fields = struct.unpack(FORMAT, header_bytes)
    if fields[0] != MAGIC or fields[1] != 1 or fields[2] != HEADER_SIZE:
        raise ValueError("invalid header identity")
    if fields[-1] != (binascii.crc32(header_bytes[:-4]) & 0xFFFFFFFF):
        raise ValueError("invalid header CRC")
    if fields[4] != TARGET_PICO2_W or fields[3] & ~FLAG_DEVELOPMENT:
        raise ValueError("unsupported target or flags")
    image_size, payload_offset, entry_offset, vector_offset = fields[5:9]
    if (image_size == 0 or image_size > MAX_PAYLOAD_SIZE or
            payload_offset != MANIFEST_SIZE or entry_offset >= image_size or
            vector_offset >= image_size or len(blob) != MANIFEST_SIZE + image_size):
        raise ValueError("invalid image bounds")
    if fields[14] > 1 or fields[15] != 1 or fields[16] != 0:
        raise ValueError("unsupported bootloader, digest, or signature")
    if fields[17] != bytes(16) or fields[19] != bytes(64):
        raise ValueError("unsigned development policy violation")
    if any(value != 0 for value in fields[21]) or any(
            value != 0xFF for value in blob[HEADER_SIZE:MANIFEST_SIZE]):
        raise ValueError("noncanonical reserved bytes")
    payload = blob[MANIFEST_SIZE:]
    if fields[18] != hashlib.sha256(payload).digest():
        raise ValueError("payload digest mismatch")


def parse_version(value: str) -> tuple[int, int, int]:
    parts = tuple(int(part) for part in value.split("."))
    if len(parts) != 3 or any(part < 0 or part > 0xFFFF for part in parts):
        raise argparse.ArgumentTypeError("version must be MAJOR.MINOR.PATCH")
    return parts


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", required=True, type=pathlib.Path)
    parser.add_argument("--elf", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--version", required=True, type=parse_version)
    parser.add_argument("--build-number", required=True, type=int)
    args = parser.parse_args()

    payload = args.bin.read_bytes()
    provenance = hashlib.sha256(args.elf.read_bytes()).digest()
    image = build_image(payload, elf_entry(args.elf), provenance,
                        args.version, args.build_number)
    args.output.write_bytes(image)


if __name__ == "__main__":
    main()
