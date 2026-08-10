#ifndef PICOPEN_WIFI_VAULT_H
#define PICOPEN_WIFI_VAULT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_VAULT_PIN_SIZE 17u

typedef enum picopen_vault_result {
    PICOPEN_VAULT_OK = 0, PICOPEN_VAULT_EMPTY, PICOPEN_VAULT_LOCKED,
    PICOPEN_VAULT_BAD_PIN, PICOPEN_VAULT_STORAGE, PICOPEN_VAULT_INVALID,
} picopen_vault_result_t;

bool picopen_wifi_vault_present(void);
picopen_vault_result_t picopen_wifi_vault_save(const char *pin, const char *ssid,
                                                const char *password);
picopen_vault_result_t picopen_wifi_vault_load(const char *pin, char *ssid,
                                                size_t ssid_size, char *password,
                                                size_t password_size, uint64_t now_ms);
bool picopen_wifi_vault_forget(bool locally_confirmed);
uint32_t picopen_wifi_vault_retry_seconds(uint64_t now_ms);
const char *picopen_wifi_vault_result_name(picopen_vault_result_t result);

#endif
