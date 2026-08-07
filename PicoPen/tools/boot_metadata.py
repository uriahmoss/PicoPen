"""Host-side encoder and validator for PicoPen boot metadata v1."""

import binascii
import struct

MAGIC = b"PPBOOT1\0"
FORMAT_VERSION = 1
RECORD_SIZE = 256
MAX_ATTEMPTS = 3
FLAG_PENDING = 1
FLAG_CONFIRMED = 2
KNOWN_FLAGS = FLAG_PENDING | FLAG_CONFIRMED
FORMAT = "<8sHHIIBBHI224sI"


def encode(generation, flags, attempts, last_failure=0):
    if flags & ~KNOWN_FLAGS or flags == KNOWN_FLAGS:
        raise ValueError("invalid boot flags")
    if not 0 <= attempts <= MAX_ATTEMPTS:
        raise ValueError("invalid boot-attempt count")
    prefix = struct.pack(FORMAT, MAGIC, FORMAT_VERSION, RECORD_SIZE,
                         generation & 0xFFFFFFFF, flags, attempts, 0, 0,
                         last_failure, bytes(224), 0)
    crc = binascii.crc32(prefix[:-4]) & 0xFFFFFFFF
    return prefix[:-4] + struct.pack("<I", crc)


def decode(record):
    if len(record) != RECORD_SIZE:
        raise ValueError("invalid metadata size")
    fields = struct.unpack(FORMAT, record)
    magic, version, size, generation, flags, attempts, slot = fields[:7]
    reserved16, last_failure, reserved, stored_crc = fields[7:]
    if magic != MAGIC or version != FORMAT_VERSION or size != RECORD_SIZE:
        raise ValueError("invalid metadata identity")
    if flags & ~KNOWN_FLAGS or flags == KNOWN_FLAGS:
        raise ValueError("invalid boot flags")
    if attempts > MAX_ATTEMPTS or slot != 0 or reserved16 or any(reserved):
        raise ValueError("invalid metadata fields")
    if stored_crc != binascii.crc32(record[:-4]) & 0xFFFFFFFF:
        raise ValueError("invalid metadata CRC")
    return {"generation": generation, "flags": flags,
            "attempts": attempts, "last_failure": last_failure}


def select(copy_a, copy_b):
    valid = []
    for record in (copy_a, copy_b):
        try:
            valid.append((decode(record), record))
        except ValueError:
            valid.append((None, record))
    if valid[0][0] is None and valid[1][0] is None:
        return None
    if valid[0][0] is None:
        return valid[1][0]
    if valid[1][0] is None:
        return valid[0][0]
    a_generation = valid[0][0]["generation"]
    b_generation = valid[1][0]["generation"]
    difference = (b_generation - a_generation) & 0xFFFFFFFF
    return valid[1][0] if 0 < difference < 0x80000000 else valid[0][0]
