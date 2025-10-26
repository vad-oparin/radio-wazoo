#ifndef NVS_H
#define NVS_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Non-Volatile Storage
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_init(void);

/**
 * @brief Store a string value in NVS cache
 *
 * @param key Key name (max 15 characters)
 * @param value String value to store
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_cache_put_str(const char *key, const char *value);

/**
 * @brief Retrieve a string value from NVS cache
 *
 * @param key Key name
 * @param value Buffer to store retrieved value
 * @param max_len Maximum length of buffer
 * @return esp_err_t ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key doesn't exist
 */
esp_err_t nvs_cache_get_str(const char *key, char *value, size_t max_len);

/**
 * @brief Store an integer value in NVS cache
 *
 * @param key Key name (max 15 characters)
 * @param value Integer value to store
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_cache_put_i32(const char *key, int32_t value);

/**
 * @brief Retrieve an integer value from NVS cache
 *
 * @param key Key name
 * @param value Pointer to store retrieved value
 * @return esp_err_t ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key doesn't exist
 */
esp_err_t nvs_cache_get_i32(const char *key, int32_t *value);

/**
 * @brief Delete a key from NVS cache
 *
 * @param key Key name to delete
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_cache_forget(const char *key);

/**
 * @brief Commit all pending changes to NVS
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_cache_flush(void);

#ifdef __cplusplus
}
#endif

#endif // NVS_H
