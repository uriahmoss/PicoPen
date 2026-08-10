#include "picopen/wifi_vault.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "mbedtls/gcm.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/platform_util.h"
#include "pico/rand.h"
#include "pico/unique_id.h"
#include "picopen/internal_fs.h"

#define VAULT_MAGIC UINT32_C(0x31545650)
#define VAULT_ITERATIONS 50000u
#define VAULT_MAX_FAILURES 5u
#define VAULT_LOCK_MS 30000u
#define VAULT_FILE "/wifi.vlt"

typedef struct vault_plaintext { char ssid[33]; char password[64]; } vault_plaintext_t;
typedef struct vault_record {
    uint32_t magic, version, iterations;
    uint8_t salt[16], nonce[12];
    uint8_t ciphertext[sizeof(vault_plaintext_t)], tag[16];
} vault_record_t;

static uint32_t failures;
static uint64_t locked_until;

static void random_bytes(uint8_t *output, size_t length) {
    while (length) {
        uint64_t value = get_rand_64();
        const size_t amount = length < sizeof(value) ? length : sizeof(value);
        memcpy(output, &value, amount); output += amount; length -= amount;
    }
}

static bool derive(const char *pin, const vault_record_t *record, uint8_t key[32]) {
    if (!pin || strlen(pin) < 4u || strlen(pin) >= PICOPEN_VAULT_PIN_SIZE) return false;
    uint8_t salt[sizeof(record->salt) + PICO_UNIQUE_BOARD_ID_SIZE_BYTES];
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    memcpy(salt, record->salt, sizeof(record->salt));
    memcpy(salt + sizeof(record->salt), board_id.id, sizeof(board_id.id));
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    const bool ok = md && mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
        (const uint8_t *)pin, strlen(pin), salt, sizeof(salt),
        record->iterations, 32u, key) == 0;
    mbedtls_platform_zeroize(salt, sizeof(salt));
    mbedtls_platform_zeroize(&board_id, sizeof(board_id));
    return ok;
}

static const uint8_t *aad(const vault_record_t *record) { return (const uint8_t *)record; }
static size_t aad_size(void) { return offsetof(vault_record_t, ciphertext); }

bool picopen_wifi_vault_present(void) {
    vault_record_t record; size_t length = 0u;
    const bool present = picopen_internal_fs_read(VAULT_FILE, &record, sizeof(record), &length) &&
                         length == sizeof(record) && record.magic == VAULT_MAGIC;
    mbedtls_platform_zeroize(&record, sizeof(record)); return present;
}

picopen_vault_result_t picopen_wifi_vault_save(const char *pin, const char *ssid,
                                                const char *password) {
    if (!ssid || !password || !ssid[0] || strlen(ssid) >= sizeof(((vault_plaintext_t *)0)->ssid) ||
        strlen(password) >= sizeof(((vault_plaintext_t *)0)->password)) return PICOPEN_VAULT_INVALID;
    vault_record_t record = {.magic = VAULT_MAGIC, .version = 1u, .iterations = VAULT_ITERATIONS};
    vault_plaintext_t plain = {0}; uint8_t key[32] = {0}; mbedtls_gcm_context gcm;
    snprintf(plain.ssid, sizeof(plain.ssid), "%s", ssid);
    snprintf(plain.password, sizeof(plain.password), "%s", password);
    random_bytes(record.salt, sizeof(record.salt)); random_bytes(record.nonce, sizeof(record.nonce));
    bool ok = derive(pin, &record, key);
    mbedtls_gcm_init(&gcm);
    ok = ok && mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256u) == 0 &&
         mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, sizeof(plain),
             record.nonce, sizeof(record.nonce), aad(&record), aad_size(),
             (const uint8_t *)&plain, record.ciphertext, sizeof(record.tag), record.tag) == 0 &&
         picopen_internal_fs_replace(VAULT_FILE, &record, sizeof(record));
    mbedtls_gcm_free(&gcm); mbedtls_platform_zeroize(key, sizeof(key));
    mbedtls_platform_zeroize(&plain, sizeof(plain)); mbedtls_platform_zeroize(&record, sizeof(record));
    return ok ? PICOPEN_VAULT_OK : PICOPEN_VAULT_STORAGE;
}

picopen_vault_result_t picopen_wifi_vault_load(const char *pin, char *ssid,
                                                size_t ssid_size, char *password,
                                                size_t password_size, uint64_t now_ms) {
    if (now_ms < locked_until) return PICOPEN_VAULT_LOCKED;
    if (!ssid || !password || ssid_size < sizeof(((vault_plaintext_t *)0)->ssid) ||
        password_size < sizeof(((vault_plaintext_t *)0)->password)) return PICOPEN_VAULT_INVALID;
    vault_record_t record; vault_plaintext_t plain = {0}; uint8_t key[32] = {0}; size_t length = 0u;
    if (!picopen_internal_fs_read(VAULT_FILE, &record, sizeof(record), &length)) return PICOPEN_VAULT_EMPTY;
    if (length != sizeof(record) || record.magic != VAULT_MAGIC || record.version != 1u ||
        record.iterations != VAULT_ITERATIONS) {
        mbedtls_platform_zeroize(&record, sizeof(record));
        return PICOPEN_VAULT_INVALID;
    }
    mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
    bool ok = derive(pin, &record, key) &&
        mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256u) == 0 &&
        mbedtls_gcm_auth_decrypt(&gcm, sizeof(plain), record.nonce, sizeof(record.nonce),
            aad(&record), aad_size(), record.tag, sizeof(record.tag),
            record.ciphertext, (uint8_t *)&plain) == 0;
    mbedtls_gcm_free(&gcm); mbedtls_platform_zeroize(key, sizeof(key));
    if (!ok || !memchr(plain.ssid, '\0', sizeof(plain.ssid)) ||
        !memchr(plain.password, '\0', sizeof(plain.password))) {
        failures++;
        if (failures >= VAULT_MAX_FAILURES) { failures = 0u; locked_until = now_ms + VAULT_LOCK_MS; }
        mbedtls_platform_zeroize(&plain, sizeof(plain)); mbedtls_platform_zeroize(&record, sizeof(record));
        return PICOPEN_VAULT_BAD_PIN;
    }
    snprintf(ssid, ssid_size, "%s", plain.ssid); snprintf(password, password_size, "%s", plain.password);
    failures = 0u; locked_until = 0u;
    mbedtls_platform_zeroize(&plain, sizeof(plain)); mbedtls_platform_zeroize(&record, sizeof(record));
    return PICOPEN_VAULT_OK;
}

bool picopen_wifi_vault_forget(bool locally_confirmed) {
    return locally_confirmed && picopen_internal_fs_remove(VAULT_FILE);
}

uint32_t picopen_wifi_vault_retry_seconds(uint64_t now_ms) {
    return now_ms >= locked_until ? 0u : (uint32_t)((locked_until - now_ms + 999u) / 1000u);
}

const char *picopen_wifi_vault_result_name(picopen_vault_result_t result) {
    static const char *const names[] = {
        "OK", "EMPTY", "LOCKED", "BAD PIN", "STORAGE", "INVALID",
    };
    return (unsigned int)result < sizeof(names) / sizeof(names[0])
        ? names[result] : "UNKNOWN";
}
