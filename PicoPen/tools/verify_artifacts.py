#!/usr/bin/env python3
"""Verify PicoPen UF2 structure, family, and assigned flash boundaries."""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import struct

UF2_SIZE = 512
UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000
ABSOLUTE_FAMILY_ID = 0xE48BFF57
RP2350_ARM_S_FAMILY_ID = 0xE48BFF59
RP2350_METADATA_ADDRESS = 0x10FFFF00

FLASH_BASE = 0x10000000
REGIONS = {
    "bootloader/picopen_bootloader.uf2": (FLASH_BASE, FLASH_BASE + 0x40000),
    "bringup/picopen_bringup.uf2": (FLASH_BASE, FLASH_BASE + 0x40000),
    "os/picopen_os.uf2": (FLASH_BASE + 0x51000, FLASH_BASE + 0x210000),
    "os/picopen_os_slot.uf2": (FLASH_BASE + 0x50000,
                                FLASH_BASE + 0x210000),
}


def verify_uf2(path: pathlib.Path, region: tuple[int, int]) -> dict:
    blob = path.read_bytes()
    if not blob or len(blob) % UF2_SIZE:
        raise ValueError(f"{path}: malformed UF2 length")
    physical_block_count = len(blob) // UF2_SIZE
    seen = set()
    ranges = []
    declared_count = None
    metadata_blocks = 0
    for offset in range(0, len(blob), UF2_SIZE):
        block = blob[offset:offset + UF2_SIZE]
        fields = struct.unpack_from("<IIIIIIII", block)
        magic0, magic1, flags, address, size, number, count, family = fields
        if (magic0, magic1) != (UF2_MAGIC_START0, UF2_MAGIC_START1):
            raise ValueError(f"{path}: invalid UF2 start magic")
        if struct.unpack_from("<I", block, 508)[0] != UF2_MAGIC_END:
            raise ValueError(f"{path}: invalid UF2 end magic")
        if not flags & UF2_FLAG_FAMILY_ID:
            raise ValueError(f"{path}: UF2 family ID is missing")
        if family == ABSOLUTE_FAMILY_ID:
            if (address != RP2350_METADATA_ADDRESS or size != 256 or
                    metadata_blocks != 0):
                raise ValueError(f"{path}: invalid RP2350 metadata block")
            metadata_blocks += 1
            continue
        if family != RP2350_ARM_S_FAMILY_ID:
            raise ValueError(f"{path}: UF2 is not RP2350 ARM Secure")
        if size == 0 or size > 476 or address + size < address:
            raise ValueError(f"{path}: invalid UF2 payload range")
        if number in seen:
            raise ValueError(f"{path}: duplicate UF2 block {number}")
        seen.add(number)
        declared_count = count if declared_count is None else declared_count
        if count != declared_count:
            raise ValueError(f"{path}: inconsistent UF2 block count")
        if address < region[0] or address + size > region[1]:
            raise ValueError(f"{path}: data lies outside assigned flash region")
        ranges.append((address, address + size))
    data_block_count = physical_block_count - metadata_blocks
    if (declared_count != data_block_count or
            seen != set(range(data_block_count))):
        raise ValueError(f"{path}: incomplete UF2 block set")
    ranges.sort()
    for previous, current in zip(ranges, ranges[1:]):
        if current[0] < previous[1]:
            raise ValueError(f"{path}: overlapping UF2 payload blocks")
    return {
        "blocks": data_block_count,
        "start": ranges[0][0],
        "end": max(end for _, end in ranges),
        "sha256": hashlib.sha256(blob).hexdigest(),
    }


def verify_build(build_dir: pathlib.Path) -> list[tuple[str, dict]]:
    results = []
    for relative, region in REGIONS.items():
        path = build_dir / relative
        if not path.is_file():
            raise ValueError(f"missing artifact: {path}")
        results.append((relative, verify_uf2(path, region)))
    return results


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()
    for name, result in verify_build(args.build_dir):
        print(f"{name}: 0x{result['start']:08x}..0x{result['end']:08x} "
              f"{result['blocks']} blocks sha256={result['sha256']}")


if __name__ == "__main__":
    main()
