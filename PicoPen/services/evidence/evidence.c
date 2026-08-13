#include "picopen/evidence.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "mbedtls/sha256.h"

#define EVIDENCE_MAX_SIZE UINT32_C(262144)
#define HASH_CHUNK_SIZE 256u
#define CAPTURE_SAMPLE_SIZE 80u

static picopen_evidence_snapshot_t current;
static picopen_storage_service_t *source;
static mbedtls_sha256_context hash;
static char string_run[33];
static size_t string_run_length;
static uint32_t parse_offset;
static bool little_endian;
static uint16_t capture_linktype;

static uint16_t read_be16(const uint8_t *bytes) {
    return (uint16_t)((uint16_t)bytes[0] << 8u | bytes[1]);
}

static uint16_t read_u16(const uint8_t *bytes, bool little) {
    return little ? (uint16_t)((uint16_t)bytes[1] << 8u | bytes[0])
                  : read_be16(bytes);
}

static uint32_t read_word(const uint8_t *bytes, bool little) {
    return little
        ? (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8u |
              (uint32_t)bytes[2] << 16u | (uint32_t)bytes[3] << 24u
        : (uint32_t)bytes[0] << 24u | (uint32_t)bytes[1] << 16u |
              (uint32_t)bytes[2] << 8u | bytes[3];
}

static bool read_at(uint32_t offset, uint8_t *data, size_t capacity,
                    size_t *length) {
    bool truncated = false;
    return picopen_storage_read_file(source, current.path, offset, data,
        capacity, length, &truncated) == PICOPEN_STORAGE_OK;
}

static void retain_string(void) {
    if (string_run_length < 4u) {
        string_run_length = 0u;
        return;
    }
    ++current.string_count;
    size_t preview_length = strlen(current.string_preview);
    if (preview_length && preview_length + 1u < sizeof(current.string_preview))
        current.string_preview[preview_length++] = ' ';
    for (size_t index = 0u; index < string_run_length &&
         preview_length + 1u < sizeof(current.string_preview); ++index) {
        current.string_preview[preview_length++] = string_run[index];
    }
    current.string_preview[preview_length] = '\0';
    string_run_length = 0u;
}

static void inspect_strings(const uint8_t *data, size_t length) {
    for (size_t index = 0u; index < length; ++index) {
        if (isprint(data[index])) {
            if (string_run_length + 1u < sizeof(string_run))
                string_run[string_run_length++] = (char)data[index];
        } else {
            retain_string();
        }
    }
}

static void inspect_ethernet(const uint8_t *packet, size_t length) {
    if (length < 14u) return;
    const uint16_t ether_type = read_be16(&packet[12]);
    if (ether_type == UINT16_C(0x0806)) {
        ++current.arp_count;
        return;
    }
    if (ether_type == UINT16_C(0x86DD)) {
        ++current.ipv6_count;
        if (length >= 21u) {
            if (packet[20] == 6u) ++current.tcp_count;
            else if (packet[20] == 17u) ++current.udp_count;
            else if (packet[20] == 58u) ++current.icmp_count;
        }
        return;
    }
    if (ether_type != UINT16_C(0x0800) || length < 24u) return;
    ++current.ipv4_count;
    if (packet[23] == 6u) ++current.tcp_count;
    else if (packet[23] == 17u) ++current.udp_count;
    else if (packet[23] == 1u) ++current.icmp_count;
}

void picopen_evidence_init(void) {
    current = (picopen_evidence_snapshot_t){
        .state = PICOPEN_EVIDENCE_IDLE,
    };
    mbedtls_sha256_init(&hash);
}

bool picopen_evidence_start(picopen_storage_service_t *storage,
                            const char *path, uint32_t size,
                            bool locally_confirmed) {
    if (!locally_confirmed || !storage || !path || !path[0] ||
        size > EVIDENCE_MAX_SIZE ||
        current.state == PICOPEN_EVIDENCE_HASHING ||
        current.state == PICOPEN_EVIDENCE_PARSING) return false;
    current = (picopen_evidence_snapshot_t){
        .state = PICOPEN_EVIDENCE_HASHING, .size = size,
    };
    snprintf(current.path, sizeof(current.path), "%s", path);
    source = storage;
    string_run_length = 0u;
    parse_offset = 0u;
    capture_linktype = 0u;
    mbedtls_sha256_free(&hash);
    mbedtls_sha256_init(&hash);
    return mbedtls_sha256_starts(&hash, 0) == 0;
}

bool picopen_evidence_cancel(void) {
    if (current.state != PICOPEN_EVIDENCE_HASHING &&
        current.state != PICOPEN_EVIDENCE_PARSING) return false;
    current.state = PICOPEN_EVIDENCE_CANCELLED;
    mbedtls_sha256_free(&hash);
    return true;
}

static void finish_hash(void) {
    uint8_t digest[32];
    retain_string();
    if (mbedtls_sha256_finish(&hash, digest) != 0) {
        current.state = PICOPEN_EVIDENCE_ERROR;
        return;
    }
    for (size_t index = 0u; index < sizeof(digest); ++index)
        snprintf(&current.sha256[index * 2u], 3u, "%02x", digest[index]);
    mbedtls_sha256_free(&hash);
    current.state = PICOPEN_EVIDENCE_PARSING;
    parse_offset = 0u;
}

static bool poll_hash(void) {
    if (current.processed >= current.size) {
        finish_hash();
        return true;
    }
    uint8_t data[HASH_CHUNK_SIZE];
    size_t length = 0u;
    const size_t remaining = current.size - current.processed;
    const size_t wanted = remaining < sizeof(data) ? remaining : sizeof(data);
    if (!read_at(current.processed, data, wanted, &length) ||
        length != wanted || mbedtls_sha256_update(&hash, data, length) != 0) {
        current.state = PICOPEN_EVIDENCE_ERROR;
        return true;
    }
    inspect_strings(data, length);
    current.processed += (uint32_t)length;
    return true;
}

static bool identify_capture(void) {
    uint8_t header[28];
    size_t length = 0u;
    if (!read_at(0u, header, sizeof(header), &length) || length < 12u) {
        current.state = current.size == 0u ? PICOPEN_EVIDENCE_COMPLETE
                                          : PICOPEN_EVIDENCE_ERROR;
        return true;
    }
    const uint32_t magic = read_word(header, true);
    if (magic == UINT32_C(0xA1B2C3D4) || magic == UINT32_C(0xD4C3B2A1)) {
        if (length < 24u) {
            current.state = PICOPEN_EVIDENCE_ERROR;
            return true;
        }
        current.capture = PICOPEN_CAPTURE_PCAP;
        little_endian = magic == UINT32_C(0xA1B2C3D4);
        capture_linktype = (uint16_t)read_word(&header[20], little_endian);
        parse_offset = 24u;
        return true;
    }
    if (magic != UINT32_C(0x0A0D0D0A)) {
        current.state = PICOPEN_EVIDENCE_COMPLETE;
        return true;
    }
    if (length < 28u) {
        current.state = PICOPEN_EVIDENCE_ERROR;
        return true;
    }
    const uint32_t byte_order = read_word(&header[8], true);
    if (byte_order == UINT32_C(0x1A2B3C4D)) little_endian = true;
    else if (byte_order == UINT32_C(0x4D3C2B1A)) little_endian = false;
    else {
        current.state = PICOPEN_EVIDENCE_ERROR;
        return true;
    }
    current.capture = PICOPEN_CAPTURE_PCAPNG;
    parse_offset = read_word(&header[4], little_endian);
    if (parse_offset < 28u || parse_offset > current.size)
        current.state = PICOPEN_EVIDENCE_ERROR;
    return true;
}

static bool poll_capture(void) {
    uint8_t header[CAPTURE_SAMPLE_SIZE];
    size_t length = 0u;
    const size_t needed = current.capture == PICOPEN_CAPTURE_PCAP ? 16u : 12u;
    if (!read_at(parse_offset, header, sizeof(header), &length) || length < needed) {
        current.state = PICOPEN_EVIDENCE_COMPLETE;
        return true;
    }
    const uint32_t type = read_word(header, little_endian);
    const uint32_t captured = current.capture == PICOPEN_CAPTURE_PCAP
        ? read_word(&header[8], little_endian) :
          (type == 6u && length >= 28u ? read_word(&header[20], little_endian) : 0u);
    const uint32_t step = current.capture == PICOPEN_CAPTURE_PCAP
        ? 16u + captured : read_word(&header[4], little_endian);
    if (step < needed || step > current.size - parse_offset) {
        current.state = PICOPEN_EVIDENCE_ERROR;
        return true;
    }
    if (current.capture == PICOPEN_CAPTURE_PCAPNG && type == 1u && length >= 10u)
        capture_linktype = read_u16(&header[8], little_endian);
    if (current.capture == PICOPEN_CAPTURE_PCAP || type == 6u) {
        ++current.packet_count;
        const size_t data_offset = current.capture == PICOPEN_CAPTURE_PCAP ? 16u : 28u;
        if (capture_linktype == 1u && length > data_offset)
            inspect_ethernet(&header[data_offset], length - data_offset);
    }
    parse_offset += step;
    if (parse_offset >= current.size) current.state = PICOPEN_EVIDENCE_COMPLETE;
    return true;
}

bool picopen_evidence_poll(void) {
    if (current.state == PICOPEN_EVIDENCE_HASHING) return poll_hash();
    if (current.state != PICOPEN_EVIDENCE_PARSING) return false;
    if (parse_offset == 0u) return identify_capture();
    return poll_capture();
}

bool picopen_evidence_digest_matches(const char *expected_hex) {
    if (!expected_hex || strlen(expected_hex) != 64u ||
        current.sha256[0] == '\0') return false;
    for (size_t index = 0u; index < 64u; ++index) {
        if (tolower((unsigned char)expected_hex[index]) != current.sha256[index])
            return false;
    }
    return true;
}

void picopen_evidence_snapshot(picopen_evidence_snapshot_t *snapshot) {
    if (snapshot) *snapshot = current;
}

const char *picopen_evidence_state_name(picopen_evidence_state_t state) {
    static const char *const names[] = {
        "IDLE", "HASHING", "PARSING", "COMPLETE", "CANCELLED", "DENIED",
        "ERROR",
    };
    return (unsigned)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}
