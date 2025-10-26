#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LittleFS filesystem
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t filesystem_init(void);

/**
 * @brief Unmount LittleFS partition
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t filesystem_deinit(void);

/**
 * @brief Read entire file contents into buffer
 *
 * @param path File path (relative to mount point or absolute)
 * @param buffer Buffer to store file contents
 * @param buffer_size Size of buffer
 * @param bytes_read Pointer to store number of bytes read (can be NULL)
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_FOUND if file doesn't exist
 */
esp_err_t filesystem_read_file(const char *path, char *buffer, size_t buffer_size, size_t *bytes_read);

/**
 * @brief Write buffer contents to file (overwrites existing file)
 *
 * @param path File path (relative to mount point or absolute)
 * @param data Data to write
 * @param data_size Size of data in bytes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t filesystem_write_file(const char *path, const char *data, size_t data_size);

/**
 * @brief Check if file exists
 *
 * @param path File path (relative to mount point or absolute)
 * @return true if file exists, false otherwise
 */
bool filesystem_file_exists(const char *path);

/**
 * @brief Delete a file
 *
 * @param path File path (relative to mount point or absolute)
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_FOUND if file doesn't exist
 */
esp_err_t filesystem_delete_file(const char *path);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_H
